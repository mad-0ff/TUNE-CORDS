# TUNE CORDS

Plugin de correction de hauteur (VST3, Windows). Tu choisis la tonalité et la gamme de l'instrumental,
le plugin détecte la voix et ramène seulement les notes fausses sur la note juste de la gamme.

## Compiler avec GitHub (rien à installer)
1. Crée un dépôt GitHub privé `TUNE-CORDS`.
2. Dézippe ce dossier, puis glisse **tout son contenu** (pas le .zip) dans le dépôt : Add file > Upload files > Commit.
   Vérifie que le dossier `.github/workflows/build.yml` est bien présent dans le dépôt.
   S'il manque : Add file > Create new file, nom `.github/workflows/build.yml`, et colle le contenu du fichier.
3. Onglet **Actions** : la compilation démarre (5 à 10 min). Quand c'est vert, ouvre le run et télécharge
   l'artefact **TUNE-CORDS-Windows** en bas de page.
4. Copie le dossier `TUNE CORDS.vst3` dans `C:\Program Files\Common Files\VST3`.
5. FL Studio : Options > Manage plugins > Find installed plugins. Ajoute TUNE CORDS sur l'insert de la voix.

`TUNE CORDS.exe` (standalone) sert à tester sans FL Studio.

## Réglages
- **Tonalité / Gamme** : celles de l'instrumental.
- **Puissance** : 0 % = rien, 100 % = note exacte.
- **Vitesse** : 0 ms = effet robot/trap, 50-150 ms = naturel.
- **Tolérance** : écart (en cents) en dessous duquel une note est jugée juste et n'est pas touchée.
- **Humanize** : ralentit la correction pour garder le vibrato et le naturel.
- **Mix** : voix corrigée / voix d'origine. **Sortie** : volume.
- **Voix** : Auto, Homme ou Femme (plage de détection).
