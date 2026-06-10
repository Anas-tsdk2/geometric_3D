# Modélisation Géométrique 3D - Rapport de Projet

Voici le compte-rendu détaillé des méthodes que j'ai implémentées au cours de ce projet.

## 📝 Méthodologie et Utilisation de l'IA

Tout au long de ce projet, j'ai adopté une méthode de travail itérative avec des commits réguliers après quasiment chaque séance de TD pour suivre ma progression.
Pour coder les différents algorithmes, je me suis référé aux supports de cours. ET aussi, j'ai utilisé l'IA Gemini comme assistant pour clarifier certains concepts.

## Exercices et Méthodes implémentées

### 1. Calcul des normales (Exercice 2)
Pour que la lumière s'affiche correctement sur les objets 3D, il a fallu calculer les normales.
* **Normale des faces :** Pour une face, j'ai pris les deux premières arêtes (vecteurs `v1` et `v2`), et j'ai fait un produit vectoriel (`v1 x v2`) puis je l'ai normalisé.
* **Normale des sommets (Smooth Shading) :** Pour lisser l'affichage, la normale d'un sommet est la moyenne des normales de toutes les faces qui le touchent. Pour parcourir toutes les faces autour du sommet, j'ai utilisé une boucle avec les pointeurs Half-Edge : `h = h->prev->twin`.

### 2. Détection de la Silhouette
Pour dessiner le contour de l'objet, je parcours toutes les arêtes du maillage. 
* Pour chaque arête, je regarde les normales des deux faces qui la partagent.
* Je calcule leur produit scalaire par rapport à la direction de la caméra. Si une face "regarde" la caméra (produit scalaire positif) et l'autre lui "tourne le dos" (produit scalaire négatif), c'est qu'on est sur le bord de l'objet tel qu'il est vu depuis la caméra. 
* La condition géométrique est donc : `dot1 * dot2 <= 0.0`.

![Silhouette](images/silhouette.png)

### 3. Triangulation (Méthode de Ear Clipping)
Pour transformer n'importe quelle face en triangles (même les faces concaves), j'ai codé la méthode des "Ear Clipping" (découpage d'oreilles).
1. Je calcule la vraie normale de la face avec la **méthode de Newell**.
2. Je regarde chaque triplet de 3 sommets consécutifs. Pour qu'il forme une "oreille" qu'on peut couper, il faut :
   * Que l'angle soit convexe (vérifié via produit vectoriel : `(u x v) * normal > 0`).
   * Qu'aucun autre sommet de la face ne se trouve à l'intérieur de ce triangle.
3. Si c'est bien une oreille, je crée une nouvelle arête diagonale pour fermer le triangle, et je le détache du reste de la face.

**Après triangulation :**
![exemple](images/triangulation.png)

![exemple](images/triangulation_bis.png)
![Autre exemple](images/triangulation_bis_bis.png)

### 4. Surface de Révolution (Génération du vase)
Fonction pour générer une forme 3D fermée en faisant tourner un profil 2D autour de l'axe Y.
* Je calcule la position des nouveaux points avec des fonctions trigonométriques selon l'angle de rotation (en tranches).
* Je relie les points générés pour créer des faces à 4 côtés (Quads). Pour que le maillage soit solide, je lie les arêtes jumelles (twins) entre elles en utilisant un dictionnaire `std::map` basé sur l'ID des deux sommets de chaque arête.

![Génération du Vase](images/vase.png)

### 5. Vérification du maillage (`verifyMesh`)
Pour certifier que les opérations (comme la simplification) ne cassent pas le maillage, j'ai codé une fonction de diagnostic. Elle boucle sur tout l'objet et vérifie :
* Que l'arête sortante d'un sommet part bien de ce sommet (`originof->source == v`).
* Que les jumeaux se pointent bien mutuellement (`twin->twin == h`).
* Que les boucles de faces sont fermées (`next->prev == h` et `prev->next == h`).
* *Note : La fonction tolère les arêtes sans twin si on est sur un bord ouvert (comme le trou au-dessus du vase), mais affiche quand même un "Warning" dans la console pour nous avertir d'un éventuel trou involontaire.*

![Console](images/console.png)

### 6. Simplification (Shortest Edge Collapse)
Pour réduire le nombre de triangles, j'écrase les arêtes les plus courtes (environ 10% des arêtes retirées par clic).
* **Choix de l'arête :** Recherche de l'arête avec la plus petite distance au carré (`dx*dx + dy*dy + dz*dz`).
* **Écrasement :** 
  1. Je déplace le premier sommet pile au milieu de l'arête.
  2. Toutes les arêtes qui pointaient sur le deuxième sommet pointent désormais sur le premier.
  3. Je supprime les deux triangles liés à l'arête en reliant les bords extérieurs entre eux (`e1->twin->twin = e2->twin`).
  4. L'algorithme force un appel à `triangulate()` avant l'exécution pour ne pas casser la logique sur des Quads (comme le vase).

**Avant Simplification :**
![Avant simplification](images/simplification.png)

**Après Simplification :**
*(Le maillage a beaucoup moins d'arêtes)*

### 7. Subdivision de Catmull-Clark
Pour lisser l'objet et augmenter sa résolution, j'ai implémenté l'algorithme de Catmull-Clark :
* Je calcule le **Point de face** : la moyenne des positions des sommets de la face.
* Je calcule le **Point d'arête** : la moyenne des 2 sommets de l'arête + les 2 points des faces adjacentes.
* Je déplace les **anciens sommets** avec la formule : `V_nouveau = (F + 2R + (n-3)*V_ancien) / n`.
* **Construction :** Au lieu de modifier les pointeurs en direct, l'ancien maillage est vidé (`clear()`) et reconstruit avec de nouveaux Quads à partir des points fraîchement calculés. Cela évite totalement les erreurs critiques de topologie Half-Edge et garantit un résultat robuste.

**Résultat après lissage Catmull-Clark :**
![Catmull-Clark](images/catmull.png)


## 🛠️ Instructions de Compilation

*Le visualiseur de maillages utilise OpenGL 3.3, SDL2 et CMake.*

### Prérequis
- CMake 3.20+
- Un compilateur C++17
- OpenGL
- Git (pour télécharger les dépendances `FetchContent`)

### Compiler le projet
Les commandes sont identiques sous Windows, Linux et macOS :

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```
L'exécutable généré s'appelle `meshviewer`. Sur les systèmes Unix, vous pouvez également simplement faire :
```bash
mkdir -p build && cd build
cmake ..
make
./meshviewer
```
