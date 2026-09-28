# Earth Verb

Reverb algorithmique (FDN 8 lignes) avec interface « globe terrestre ».

- Gros bouton central = Dry/Wet : plus le Wet monte, plus le globe passe de la nuit au jour
- Haut gauche : TIME (durée de la queue, 0,1 – 20 s)
- Haut droite : DELAY (pré-delay, 0 – 500 ms)
- Bas gauche : SIZE (taille de la pièce)
- Bas droite : DECAY (absorption des aigus : plus haut = queue plus sombre)
- Double-clic sur un bouton = valeur par défaut

## Compiler (Windows + Mac, automatique, sans rien installer)
1. Créez un dépôt GitHub vide et poussez-y le contenu de ce dossier.
2. Onglet **Actions** > « Build Earth Verb » (il se lance tout seul au push).
3. Téléchargez les artefacts `EarthVerb-Windows` et `EarthVerb-macOS`.

## Compiler en local
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
Résultat dans `build/EarthVerb_artefacts/Release/` (VST3, + AU sur Mac).

## Installation
- Windows : copier `Earth Verb.vst3` dans `C:\Program Files\Common Files\VST3`
- Mac VST3 : `~/Library/Audio/Plug-Ins/VST3` — Mac AU (Logic) : `~/Library/Audio/Plug-Ins/Components`
- Mac, si le plugin est bloqué : `xattr -cr "/chemin/vers/Earth Verb.vst3"`
