// TUNE CORDS - moteur de correction de hauteur (sans dépendance JUCE)
#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <atomic>

namespace tc
{
//==============================================================================
// Gammes : masque de 12 bits (bit i = intervalle de i demi-tons depuis la tonique)
inline int makeMask (std::initializer_list<int> degrees)
{
    int m = 0;
    for (int d : degrees) m |= (1 << d);
    return m;
}

inline const char* const* scaleNames()
{
    static const char* n[] = { "Chromatique", "Majeur", "Mineur", "Mineur harmonique",
                               "Pentatonique majeur", "Pentatonique mineur", "Blues" };
    return n;
}
constexpr int numScales = 7;

inline int scaleMask (int index)
{
    switch (index)
    {
        case 0:  return 0xFFF;
        case 1:  return makeMask ({ 0, 2, 4, 5, 7, 9, 11 });
        case 2:  return makeMask ({ 0, 2, 3, 5, 7, 8, 10 });
        case 3:  return makeMask ({ 0, 2, 3, 5, 7, 8, 11 });
        case 4:  return makeMask ({ 0, 2, 4, 7, 9 });
        case 5:  return makeMask ({ 0, 3, 5, 7, 10 });
        case 6:  return makeMask ({ 0, 3, 5, 6, 7, 10 });
        default: return 0xFFF;
    }
}

inline bool noteInScale (int midiNote, int root, int mask)
{
    int degree = ((midiNote - root) % 12 + 12) % 12;
    return (mask >> degree) & 1;
}

//==============================================================================
// Détecteur de hauteur YIN
class YinDetector
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        windowSize = sampleRate > 60000.0 ? 4096 : 2048;
        hop = windowSize / 4;
        ring.assign ((size_t) windowSize, 0.0f);
        window.assign ((size_t) windowSize, 0.0f);
        diff.assign ((size_t) windowSize / 2 + 1, 0.0f);
        cmnd.assign ((size_t) windowSize / 2 + 1, 0.0f);
        writePos = 0;
        sinceLast = 0;
    }

    int getHop() const { return hop; }

    void push (float s)
    {
        ring[(size_t) writePos] = s;
        writePos = (writePos + 1) % windowSize;
        ++sinceLast;
    }

    bool frameReady() const { return sinceLast >= hop; }

    // Retourne la fréquence (Hz) ou 0 si non voisé.
    float analyse (float fmin, float fmax)
    {
        sinceLast = 0;
        // copie ordonnée (le plus ancien en premier)
        for (int i = 0; i < windowSize; ++i)
            window[(size_t) i] = ring[(size_t) ((writePos + i) % windowSize)];

        double energy = 0.0;
        for (int i = 0; i < windowSize; ++i) energy += (double) window[(size_t) i] * window[(size_t) i];
        double rms = std::sqrt (energy / windowSize);
        if (rms < 0.0015) return 0.0f; // ~ -56 dBFS : silence

        const int half = windowSize / 2;
        const int tauMax = std::min (half, (int) (sr / fmin));
        const int tauMin = std::max (2, (int) (sr / fmax));
        if (tauMax <= tauMin + 2) return 0.0f;

        const float* x = window.data();
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float sum = 0.0f;
            const float* a = x;
            const float* b = x + tau;
            for (int j = 0; j < half; ++j)
            {
                float d = a[j] - b[j];
                sum += d * d;
            }
            diff[(size_t) tau] = sum;
        }

        cmnd[0] = 1.0f;
        float running = 0.0f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            running += diff[(size_t) tau];
            cmnd[(size_t) tau] = running > 0.0f ? diff[(size_t) tau] * (float) tau / running : 1.0f;
        }

        const float threshold = 0.18f;
        int best = -1;
        for (int tau = tauMin; tau <= tauMax; ++tau)
        {
            if (cmnd[(size_t) tau] < threshold)
            {
                while (tau + 1 <= tauMax && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        }
        if (best < 0) return 0.0f;

        float tauF = (float) best;
        if (best > 1 && best < tauMax)
        {
            float s0 = cmnd[(size_t) best - 1], s1 = cmnd[(size_t) best], s2 = cmnd[(size_t) best + 1];
            float denom = s0 + s2 - 2.0f * s1;
            if (std::abs (denom) > 1e-9f) tauF += 0.5f * (s0 - s2) / denom;
        }
        return (float) (sr / tauF);
    }

private:
    double sr = 44100.0;
    int windowSize = 2048, hop = 512, writePos = 0, sinceLast = 0;
    std::vector<float> ring, window, diff, cmnd;
};

//==============================================================================
struct Params
{
    int   root       = 0;      // 0 = Do ... 11 = Si
    int   scale      = 1;      // index de gamme
    float amount     = 1.0f;   // 0..1
    float retuneMs   = 20.0f;  // vitesse de correction
    float toleranceC = 15.0f;  // zone morte en cents
    float humanize   = 0.0f;   // 0..1
    int   voice      = 0;      // 0 auto, 1 homme, 2 femme
    float mix        = 1.0f;   // 0..1
    float outputGain = 1.0f;   // linéaire
};

//==============================================================================
class PitchCorrector
{
public:
    static constexpr int kMaxChannels = 2;

    void prepare (double sampleRate, int channels)
    {
        sr = sampleRate;
        numCh = std::min (channels, kMaxChannels);
        yin.prepare (sr);
        bufSize = 1;
        while (bufSize < (int) (sr * 0.2)) bufSize <<= 1; // ~200 ms
        mask = bufSize - 1;
        for (int c = 0; c < kMaxChannels; ++c)
        {
            delayBuf[c].assign ((size_t) bufSize, 0.0f);
            dryBuf[c].assign ((size_t) bufSize, 0.0f);
        }
        writeIdx = 0;
        nominalW = (float) (4.0 * sr / 180.0);
        latency = (int) (0.5f * nominalW);
        reset();
    }

    void reset()
    {
        for (int c = 0; c < kMaxChannels; ++c)
        {
            std::fill (delayBuf[c].begin(), delayBuf[c].end(), 0.0f);
            std::fill (dryBuf[c].begin(), dryBuf[c].end(), 0.0f);
        }
        phase0 = 0.5f; gs0 = 1.0f; gs1 = 0.0f; correcting = false; W0 = W1 = Wtarget = nominalW;
        shiftSemis = desiredShift = 0.0f;
        histCount = 0; unvoicedFrames = 0; lastTarget = -1; voiced = false;
        detectedMidi.store (-1.0f); targetMidiOut.store (-1.0f);
    }

    int getLatencySamples() const { return latency; }
    void setParams (const Params& p) { params = p; }

    // Pour l'affichage (thread-safe)
    std::atomic<float> detectedMidi { -1.0f };   // hauteur détectée (note MIDI fractionnaire), -1 si rien
    std::atomic<float> targetMidiOut { -1.0f };  // note cible, -1 si rien
    std::atomic<float> shiftCentsOut { 0.0f };   // correction appliquée en cents

    void process (float** data, int numSamples)
    {
        const int ch = numCh;
        const float retuneTau = std::max (2.0f, params.retuneMs) + 80.0f * params.humanize;
        const float smoothCoef = 1.0f - std::exp (-1.0f / (0.001f * retuneTau * (float) sr));
        const float gainCoef   = 1.0f - std::exp (-1.0f / (0.003f * (float) sr));

        for (int n = 0; n < numSamples; ++n)
        {
            // --- entrée mono pour la détection
            float mono = 0.0f;
            for (int c = 0; c < ch; ++c) mono += data[c][n];
            mono /= (float) ch;
            yin.push (mono);

            if (yin.frameReady()) detectFrame();

            // --- lissage de la correction
            shiftSemis += smoothCoef * (desiredShift - shiftSemis);
            const float ratio = std::exp2 (shiftSemis / 12.0f);

            // --- écriture dans les lignes à retard
            for (int c = 0; c < ch; ++c)
            {
                delayBuf[c][(size_t) writeIdx] = data[c][n];
                dryBuf[c][(size_t) writeIdx]   = data[c][n];
            }

            // --- phases des deux têtes de lecture
            float p0 = phase0;
            float p1 = p0 + 0.5f; if (p1 >= 1.0f) p1 -= 1.0f;

            // gains visés : fondu enchaîné sin² si on corrige, sinon une seule tête (pas de filtre en peigne)
            float gt0 = sinSq (p0), gt1 = sinSq (p1);
            const float absShift = std::abs (shiftSemis);
            if (correcting && absShift < 0.02f) correcting = false;
            else if (!correcting && absShift > 0.04f) correcting = true;
            if (!correcting)
            {
                const bool first = gt0 >= gt1;
                gt0 = first ? 1.0f : 0.0f;
                gt1 = first ? 0.0f : 1.0f;
            }
            gs0 += gainCoef * (gt0 - gs0);
            gs1 += gainCoef * (gt1 - gs1);

            if (!correcting)
            {
                if (gs0 < 0.001f) W0 = Wtarget;   // la tête inaudible prend la nouvelle période
                if (gs1 < 0.001f) W1 = Wtarget;
            }

            const float d0 = std::max (2.0f, p0 * W0);
            const float d1 = std::max (2.0f, p1 * W1);

            for (int c = 0; c < ch; ++c)
            {
                float wet = gs0 * readCubic (delayBuf[c], d0) + gs1 * readCubic (delayBuf[c], d1);
                float dry = dryBuf[c][(size_t) ((writeIdx - latency) & mask)];
                float out = dry * (1.0f - params.mix) + wet * params.mix;
                data[c][n] = out * params.outputGain;
            }

            // --- avance de la phase (seulement quand on corrige)
            if (correcting)
            {
                float dphase = (1.0f - ratio) / Wtarget;
                float np = phase0 + dphase;
                bool wrapped0 = false, wrapped1 = false;
                if (np >= 1.0f) { np -= 1.0f; wrapped0 = true; }
                else if (np < 0.0f) { np += 1.0f; wrapped0 = true; }
                if (!wrapped0 && ((phase0 < 0.5f && np >= 0.5f) || (phase0 >= 0.5f && np < 0.5f)))
                    wrapped1 = true;
                phase0 = np;
                if (wrapped0) W0 = Wtarget;
                if (wrapped1) W1 = Wtarget;
            }

            writeIdx = (writeIdx + 1) & mask;
        }
        shiftCentsOut.store (shiftSemis * 100.0f);
    }

private:
    static float sinSq (float p) { float s = std::sin (3.14159265f * p); return s * s; }

    float readCubic (const std::vector<float>& buf, float delay) const
    {
        float pos = (float) writeIdx - delay;
        int i = (int) std::floor (pos);
        float f = pos - (float) i;
        float ym1 = buf[(size_t) ((i - 1) & mask)];
        float y0  = buf[(size_t) ((i)     & mask)];
        float y1  = buf[(size_t) ((i + 1) & mask)];
        float y2  = buf[(size_t) ((i + 2) & mask)];
        float c0 = y0;
        float c1 = 0.5f * (y1 - ym1);
        float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
        return ((c3 * f + c2) * f + c1) * f + c0;
    }

    void detectFrame()
    {
        float fmin = 70.0f, fmax = 900.0f;
        if (params.voice == 1) { fmin = 65.0f;  fmax = 450.0f; }
        if (params.voice == 2) { fmin = 120.0f; fmax = 1000.0f; }

        float f0 = yin.analyse (fmin, fmax);
        if (f0 > 0.0f)
        {
            unvoicedFrames = 0;
            voiced = true;
            float midi = 69.0f + 12.0f * std::log2 (f0 / 440.0f);
            hist[histCount % 3] = midi; ++histCount;
            float m = midi;
            if (histCount >= 3)
            {
                float a = hist[0], b = hist[1], c = hist[2];
                m = std::max (std::min (a, b), std::min (std::max (a, b), c)); // médiane
            }

            const int sm = scaleMask (params.scale);
            int centre = (int) std::lround (m);
            int best = -1; float bestDist = 1e9f;
            for (int cand = centre - 6; cand <= centre + 6; ++cand)
            {
                if (!noteInScale (cand, params.root, sm)) continue;
                float dist = std::abs ((float) cand - m);
                if (dist < bestDist) { bestDist = dist; best = cand; }
            }
            // hystérésis : on garde l'ancienne note cible si elle reste presque aussi proche
            if (lastTarget >= 0 && noteInScale (lastTarget, params.root, sm)
                && std::abs ((float) lastTarget - m) < bestDist + 0.2f)
                best = lastTarget;
            lastTarget = best;

            float dev = (float) best - m;            // demi-tons, >0 = trop grave
            float devC = std::abs (dev) * 100.0f;
            float knee = std::clamp ((devC - params.toleranceC) / std::max (10.0f, params.toleranceC), 0.0f, 1.0f);
            desiredShift = dev * params.amount * knee;

            detectedMidi.store (m);
            targetMidiOut.store ((float) best);

            // la fenêtre du tampon suit la période détectée (4 périodes)
            float wt = 4.0f * (float) sr / f0;
            Wtarget = std::clamp (wt, 4.0f * (float) sr / 1000.0f, 4.0f * (float) sr / 65.0f);
        }
        else
        {
            if (++unvoicedFrames >= 2)
            {
                voiced = false;
                desiredShift = 0.0f;
                lastTarget = -1;
                histCount = 0;
                detectedMidi.store (-1.0f);
                targetMidiOut.store (-1.0f);
            }
        }
    }

    Params params;
    YinDetector yin;
    double sr = 44100.0;
    int numCh = 2, bufSize = 16384, mask = 16383, writeIdx = 0, latency = 0;
    std::vector<float> delayBuf[kMaxChannels], dryBuf[kMaxChannels];
    float gs0 = 1.0f, gs1 = 0.0f;
    bool correcting = false;
    float phase0 = 0.0f, W0 = 1000.0f, W1 = 1000.0f, Wtarget = 1000.0f, nominalW = 1000.0f;
    float shiftSemis = 0.0f, desiredShift = 0.0f;
    float hist[3] = { 0, 0, 0 };
    int histCount = 0, unvoicedFrames = 0, lastTarget = -1;
    bool voiced = false;
};

} // namespace tc
