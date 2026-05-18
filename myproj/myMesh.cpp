#include "myMesh.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <cstdlib>
#include <GL/glew.h>
#include "myVector3D.h"

using namespace std;

myMesh::myMesh(void)
{
}


myMesh::~myMesh(void)
{
	clear();
}

void myMesh::clear()
{
	for (unsigned int i = 0; i < vertices.size(); i++) if (vertices[i]) delete vertices[i];
	for (unsigned int i = 0; i < halfedges.size(); i++) if (halfedges[i]) delete halfedges[i];
	for (unsigned int i = 0; i < faces.size(); i++) if (faces[i]) delete faces[i];

	vector<myVertex *> empty_vertices;    vertices.swap(empty_vertices);
	vector<myHalfedge *> empty_halfedges; halfedges.swap(empty_halfedges);
	vector<myFace *> empty_faces;         faces.swap(empty_faces);
}

void myMesh::checkMesh()
{
	vector<myHalfedge *>::iterator it;
	for (it = halfedges.begin(); it != halfedges.end(); it++)
	{
		if ((*it)->twin == NULL)
			break;
	}
	if (it != halfedges.end())
		cout << "Error! Not all edges have their twins!\n";
	else cout << "Each edge has a twin!\n";
}

void myMesh::verifyMesh()
{
	cout << "--- Debut des tests de la structure Half-Edge ---" << endl;
	bool global_ok = true;

	// 1. Verification des sommets
	bool sommets_ok = true;
	for (unsigned int i = 0; i < vertices.size(); i++) {
		myVertex* v = vertices[i];
		if (v->originof != NULL) {
			// l'arete qui part de ce sommet doit bien avoir ce sommet comme depart
			if (v->originof->source != v) {
				cout << "Erreur : le sommet " << i << " a un pointeur originof faux !" << endl;
				sommets_ok = false;
				global_ok = false;
			}
		}
	}
	if (sommets_ok) cout << "-> Test des sommets : VALIDE" << endl;
	else cout << "-> Test des sommets : ECHEC" << endl;

	// 2. Verification des demi-aretes (les semi-aretes)
	bool aretes_ok = true;
	for (unsigned int i = 0; i < halfedges.size(); i++) {
		myHalfedge* h = halfedges[i];
		
		// test du twin (le jumeau)
		if (h->twin != NULL) {
			if (h->twin->twin != h) {
				cout << "Erreur : le twin de l'arete " << i << " ne pointe pas vers elle en retour !" << endl;
				aretes_ok = false;
				global_ok = false;
			}
		} else {
			cout << "Erreur : l'arete " << i << " n'a pas de jumeau (twin null) !" << endl;
			aretes_ok = false;
			global_ok = false;
		}

		// test de la continuite (next et prev)
		if (h->next != NULL && h->next->prev != h) {
			cout << "Erreur : next->prev est casse pour l'arete " << i << endl;
			aretes_ok = false;
			global_ok = false;
		}
		if (h->prev != NULL && h->prev->next != h) {
			cout << "Erreur : prev->next est casse pour l'arete " << i << endl;
			aretes_ok = false;
			global_ok = false;
		}
	}
	if (aretes_ok) cout << "-> Test des demi-aretes : VALIDE" << endl;
	else cout << "-> Test des demi-aretes : ECHEC" << endl;

	// 3. Verification des faces
	bool faces_ok = true;
	for (unsigned int i = 0; i < faces.size(); i++) {
		myFace* f = faces[i];
		if (f->adjacent_halfedge != NULL) {
			// on s'assure que l'arete de la face pointe bien sur nous
			if (f->adjacent_halfedge->adjacent_face != f) {
				cout << "Erreur : l'arete de la face " << i << " ne reconnait pas sa face !" << endl;
				faces_ok = false;
				global_ok = false;
			}
		} else {
			cout << "Erreur : la face " << i << " est vide (pointeur null) !" << endl;
			faces_ok = false;
			global_ok = false;
		}
	}
	if (faces_ok) cout << "-> Test des faces : VALIDE" << endl;
	else cout << "-> Test des faces : ECHEC" << endl;

	if (global_ok) {
		cout << "Super, aucun probleme detecte dans tout le maillage !" << endl;
	} else {
		cout << "Attention : La structure est cassee quelque part..." << endl;
	}
	cout << "-------------------------------------------------" << endl;
}


bool myMesh::readFile(std::string filename)
{
	clear();

	ifstream fin(filename);
	if (!fin.is_open()) {
		cout << "Unable to open file!\n";
		return false;
	}
	name = filename;

	map<pair<int, int>, myHalfedge *> twin_map;
	string s, t, u;

	while (getline(fin, s))
	{
		if (s.empty()) continue;
		stringstream myline(s);
		myline >> t;
		if (t.empty() || t[0] == '#') {}
		else if (t == "g") {}
		else if (t == "v")
		{
			double x, y, z;
			if (!(myline >> x >> y >> z)) continue;

			myVertex *v = new myVertex();
			v->point = new myPoint3D(x, y, z);
			v->index = static_cast<int>(vertices.size());
			vertices.push_back(v);
		}
		else if (t == "mtllib") {}
		else if (t == "usemtl") {}
		else if (t == "s") {}
		else if (t == "f")
		{
			vector<int> faceids;
			while (myline >> u)
			{
				size_t sep = u.find('/');
				string vid = (sep == string::npos) ? u : u.substr(0, sep);
				if (vid.empty()) continue;

				int idx = atoi(vid.c_str());
				if (idx > 0) idx -= 1;
				else if (idx < 0) idx = static_cast<int>(vertices.size()) + idx;
				else continue;

				if (idx < 0 || idx >= static_cast<int>(vertices.size())) continue;
				faceids.push_back(idx);
			}

			if (faceids.size() < 3) continue;

			myFace *f = new myFace();
			faces.push_back(f);

			vector<myHalfedge *> face_halfedges(faceids.size(), NULL);

			for (size_t i = 0; i < faceids.size(); i++)
			{
				myHalfedge *h = new myHalfedge();
				h->source = vertices[faceids[i]];
				h->adjacent_face = f;
				h->index = static_cast<int>(halfedges.size());
				halfedges.push_back(h);
				face_halfedges[i] = h;

				if (h->source != NULL && h->source->originof == NULL)
					h->source->originof = h;
			}

			for (size_t i = 0; i < face_halfedges.size(); i++)
			{
				face_halfedges[i]->next = face_halfedges[(i + 1) % face_halfedges.size()];
				face_halfedges[i]->prev = face_halfedges[(i + face_halfedges.size() - 1) % face_halfedges.size()];
			}
			f->adjacent_halfedge = face_halfedges[0];

			for (size_t i = 0; i < faceids.size(); i++)
			{
				const int from = faceids[i];
				const int to = faceids[(i + 1) % faceids.size()];

				pair<int, int> reverse_edge(to, from);
				map<pair<int, int>, myHalfedge *>::iterator twin_it = twin_map.find(reverse_edge);
				if (twin_it != twin_map.end())
				{
					face_halfedges[i]->twin = twin_it->second;
					twin_it->second->twin = face_halfedges[i];
					twin_map.erase(twin_it);
				}
				else
				{
					twin_map[pair<int, int>(from, to)] = face_halfedges[i];
				}
			}
		}
	}

	if (vertices.empty() || faces.empty())
	{
		cout << "OBJ contained no usable geometry.\n";
		return false;
	}

	checkMesh();
	normalize();

	return true;
}

void myMesh::generateSurfaceOfRevolution(std::vector<myPoint3D*> profile, int slices)
{
	clear();
	if (profile.empty() || slices < 3) return;

	int nbPts = profile.size();
	
	// etape 1: on cree tous les points en tournant autour de Y
	double angle = 2.0 * 3.14159265 / slices;
	myVector3D axeY(0.0, 1.0, 0.0);

	for (int i = 0; i < slices; i++) {
		for (int j = 0; j < nbPts; j++) {
			myPoint3D* p = new myPoint3D(profile[j]->X, profile[j]->Y, profile[j]->Z);
			p->rotate(axeY, i * angle); // on fait tourner le point
			
			myVertex* v = new myVertex();
			v->point = p;
			v->index = vertices.size(); // pas besoin de static_cast je pense
			vertices.push_back(v);
		}
	}

	// etape 2: on relie les points pour faire les faces (des quads)
	map<pair<int, int>, myHalfedge*> twin_map; // pour gerer les twins

	for (int i = 0; i < slices; i++) {
		int next_i = (i + 1) % slices;
		
		for (int j = 0; j < nbPts - 1; j++) {
			// on recupere les 4 coins du quad
			int p1 = i * nbPts + j;
			int p2 = next_i * nbPts + j;
			int p3 = next_i * nbPts + (j + 1);
			int p4 = i * nbPts + (j + 1);

			myFace* f = new myFace();
			faces.push_back(f);

			// on fait les 4 demi aretes "a la main"
			myHalfedge* h1 = new myHalfedge();
			myHalfedge* h2 = new myHalfedge();
			myHalfedge* h3 = new myHalfedge();
			myHalfedge* h4 = new myHalfedge();

			h1->source = vertices[p1];
			h2->source = vertices[p2];
			h3->source = vertices[p3];
			h4->source = vertices[p4];

			h1->adjacent_face = f;
			h2->adjacent_face = f;
			h3->adjacent_face = f;
			h4->adjacent_face = f;

			h1->index = halfedges.size(); halfedges.push_back(h1);
			h2->index = halfedges.size(); halfedges.push_back(h2);
			h3->index = halfedges.size(); halfedges.push_back(h3);
			h4->index = halfedges.size(); halfedges.push_back(h4);

			// lier le sommet a son arete si c'est pas fait
			if (h1->source->originof == NULL) h1->source->originof = h1;
			if (h2->source->originof == NULL) h2->source->originof = h2;
			if (h3->source->originof == NULL) h3->source->originof = h3;
			if (h4->source->originof == NULL) h4->source->originof = h4;

			// next et prev
			h1->next = h2; h1->prev = h4;
			h2->next = h3; h2->prev = h1;
			h3->next = h4; h3->prev = h2;
			h4->next = h1; h4->prev = h3;

			f->adjacent_halfedge = h1;

			// gestion des twins
			myHalfedge* tab_h[4] = {h1, h2, h3, h4};
			int tab_id[4] = {p1, p2, p3, p4};

			for(int k = 0; k < 4; k++) {
				int de = tab_id[k];
				int vers = tab_id[(k+1)%4];
				
				pair<int, int> envers(vers, de);
				if (twin_map.count(envers)) {
					// on a trouve le twin !
					tab_h[k]->twin = twin_map[envers];
					twin_map[envers]->twin = tab_h[k];
					twin_map.erase(envers);
				} else {
					twin_map[pair<int, int>(de, vers)] = tab_h[k];
				}
			}
		}
	}

	normalize(); // on centre tout
	computeNormals();
}


void myMesh::computeNormals()
{
	for(unsigned int i=0; i<faces.size(); i++) {
		faces[i]->computeNormal();
	}
	for(unsigned int i=0; i<vertices.size(); i++) {
		vertices[i]->computeNormal();
	}
}

void myMesh::normalize()
{
	if (vertices.size() < 1) return;

	int tmpxmin = 0, tmpymin = 0, tmpzmin = 0, tmpxmax = 0, tmpymax = 0, tmpzmax = 0;

	for (unsigned int i = 0; i < vertices.size(); i++) {
		if (vertices[i]->point->X < vertices[tmpxmin]->point->X) tmpxmin = i;
		if (vertices[i]->point->X > vertices[tmpxmax]->point->X) tmpxmax = i;

		if (vertices[i]->point->Y < vertices[tmpymin]->point->Y) tmpymin = i;
		if (vertices[i]->point->Y > vertices[tmpymax]->point->Y) tmpymax = i;

		if (vertices[i]->point->Z < vertices[tmpzmin]->point->Z) tmpzmin = i;
		if (vertices[i]->point->Z > vertices[tmpzmax]->point->Z) tmpzmax = i;
	}

	double xmin = vertices[tmpxmin]->point->X, xmax = vertices[tmpxmax]->point->X,
		ymin = vertices[tmpymin]->point->Y, ymax = vertices[tmpymax]->point->Y,
		zmin = vertices[tmpzmin]->point->Z, zmax = vertices[tmpzmax]->point->Z;

	double scale = (xmax - xmin) > (ymax - ymin) ? (xmax - xmin) : (ymax - ymin);
	scale = scale > (zmax - zmin) ? scale : (zmax - zmin);

	for (unsigned int i = 0; i < vertices.size(); i++) {
		vertices[i]->point->X -= (xmax + xmin) / 2;
		vertices[i]->point->Y -= (ymax + ymin) / 2;
		vertices[i]->point->Z -= (zmax + zmin) / 2;

		vertices[i]->point->X /= scale;
		vertices[i]->point->Y /= scale;
		vertices[i]->point->Z /= scale;
	}
}


void myMesh::splitFaceTRIS(myFace *f, myPoint3D *p)
{
	/**** TODO ****/
}

void myMesh::splitEdge(myHalfedge *e1, myPoint3D *p)
{

	/**** TODO ****/
}

void myMesh::splitFaceQUADS(myFace *f, myPoint3D *p)
{
	/**** TODO ****/
}


void myMesh::subdivisionCatmullClark()
{
	/**** TODO ****/
}

void myMesh::collapseEdge(myHalfedge* e)
{
	// On verifie que l'arete existe et qu'elle n'est pas au bord
	if (e == NULL || e->twin == NULL) return; 
	
	myVertex* v1 = e->source;
	myVertex* v2 = e->twin->source;

	if (v1 == NULL || v2 == NULL) return;

	// On bouge v1 pile au milieu des deux
	v1->point->X = (v1->point->X + v2->point->X) / 2.0;
	v1->point->Y = (v1->point->Y + v2->point->Y) / 2.0;
	v1->point->Z = (v1->point->Z + v2->point->Z) / 2.0;

	// Toutes les aretes qui partaient de v2 partent maintenant de v1
	for (unsigned int i = 0; i < halfedges.size(); i++) {
		if (halfedges[i]->source == v2) {
			halfedges[i]->source = v1;
		}
	}

	// On recupere les aretes autour (les 2 triangles qui vont disparaitre)
	myHalfedge* e1 = e->next;
	myHalfedge* e2 = e->prev;
	myHalfedge* e3 = e->twin->next;
	myHalfedge* e4 = e->twin->prev;

	// On relie les faces exterieures entre elles pour boucher le trou
	if (e1 != NULL && e2 != NULL && e1->twin != NULL && e2->twin != NULL) {
		e1->twin->twin = e2->twin;
		e2->twin->twin = e1->twin;
	}

	if (e3 != NULL && e4 != NULL && e3->twin != NULL && e4->twin != NULL) {
		e3->twin->twin = e4->twin;
		e4->twin->twin = e3->twin;
	}

	// On corrige les faces pour qu'elles ne pointent plus sur les aretes supprimees
	if (e1 != NULL && e1->twin != NULL) e1->twin->adjacent_face->adjacent_halfedge = e1->twin;
	if (e2 != NULL && e2->twin != NULL) e2->twin->adjacent_face->adjacent_halfedge = e2->twin;
	if (e3 != NULL && e3->twin != NULL) e3->twin->adjacent_face->adjacent_halfedge = e3->twin;
	if (e4 != NULL && e4->twin != NULL) e4->twin->adjacent_face->adjacent_halfedge = e4->twin;

	// v1 doit pointer sur une arete valide
	if (e2 != NULL && e2->twin != NULL) v1->originof = e2->twin;

	// On s'assure que les autres sommets du triangle ne pointent pas sur les aretes qui disparaissent
	if (e2 != NULL && e1 != NULL && e1->twin != NULL && e2->source->originof == e2) {
		e2->source->originof = e1->twin;
	}
	if (e4 != NULL && e3 != NULL && e3->twin != NULL && e4->source->originof == e4) {
		e4->source->originof = e3->twin;
	}

	// On supprime l'ancien point v2 de notre liste
	for (unsigned int i = 0; i < vertices.size(); i++) {
		if (vertices[i] == v2) {
			vertices.erase(vertices.begin() + i);
			break;
		}
	}

	// On supprime les 2 faces ecrasees
	myFace* f1 = e->adjacent_face;
	myFace* f2 = e->twin->adjacent_face;
	for (unsigned int i = 0; i < faces.size(); i++) {
		if (faces[i] == f1 || faces[i] == f2) { 
			faces.erase(faces.begin() + i); 
			i--; 
		}
	}

	// On supprime les 6 aretes internes qui ont disparu
	myHalfedge* liste[6] = {e, e->twin, e1, e2, e3, e4};
	for (int k = 0; k < 6; k++) {
		if (liste[k] == NULL) continue;
		for (unsigned int i = 0; i < halfedges.size(); i++) {
			if (halfedges[i] == liste[k]) {
				halfedges.erase(halfedges.begin() + i);
				break;
			}
		}
	}
}

void myMesh::simplify()
{
	cout << "Simplification en cours..." << endl;
	
	// La simplification suppose que ce sont des triangles. On s'en assure :
	triangulate();
	
	// On enleve 10% du maillage
	int limite = halfedges.size() * 0.1; 
	
	for (int k = 0; k < limite; k++) {
		
		double min_dist = 999999.0;
		myHalfedge* arete_courte = NULL;

		// On cherche l'arete la plus petite
		for (unsigned int i = 0; i < halfedges.size(); i++) {
			myHalfedge* h = halfedges[i];
			if (h == NULL || h->twin == NULL) continue; // on passe les bords

			// Calcul de la distance
			double dx = h->source->point->X - h->twin->source->point->X;
			double dy = h->source->point->Y - h->twin->source->point->Y;
			double dz = h->source->point->Z - h->twin->source->point->Z;
			double dist = dx*dx + dy*dy + dz*dz;

			if (dist < min_dist) {
				min_dist = dist;
				arete_courte = h;
			}
		}

		// On ecrase la plus petite
		if (arete_courte != NULL) {
			collapseEdge(arete_courte);
		}
	}

	// On recale les id pour OpenGL
	for(unsigned int i = 0; i < vertices.size(); i++) vertices[i]->index = i;
	for(unsigned int i = 0; i < halfedges.size(); i++) halfedges[i]->index = i;

	cout << "C'est fini ! J'ai enleve " << limite << " aretes." << endl;
}


void myMesh::triangulate()
{
	for (size_t i = 0; i < faces.size(); i++)
	{
		triangulate(faces[i]);
	}
}

bool myMesh::triangulate(myFace *f)
{
	if (f == NULL || f->adjacent_halfedge == NULL) return false;

	vector<myHalfedge*> edges;
	myHalfedge* curr = f->adjacent_halfedge;
	do {
		edges.push_back(curr);
		curr = curr->next;
	} while (curr != f->adjacent_halfedge && curr != NULL);

	if (edges.size() <= 3) return false;

	// Calcul de la normale du polygone
	myVector3D normal(0, 0, 0);
	for (size_t i = 0; i < edges.size(); i++) {
		myPoint3D* p1 = edges[i]->source->point;
		myPoint3D* p2 = edges[(i + 1) % edges.size()]->source->point;
		normal.dX += (p1->Y - p2->Y) * (p1->Z + p2->Z);
		normal.dY += (p1->Z - p2->Z) * (p1->X + p2->X);
		normal.dZ += (p1->X - p2->X) * (p1->Y + p2->Y);
	}
	normal.normalize();

	while (edges.size() > 3) {
		bool earFound = false;
		int n = edges.size();

		for (int i = 0; i < n; i++) {
			int prev = (i - 1 + n) % n;
			int next = (i + 1) % n;

			//[Vi-1;Vi; Vi+1] 
			myPoint3D* Vi_minus_1 = edges[prev]->source->point;
			myPoint3D* Vi         = edges[i]->source->point;
			myPoint3D* Vi_plus_1  = edges[next]->source->point;

			// if Vi is convexe : 
			myVector3D u = *Vi - *Vi_minus_1;
			myVector3D v = *Vi_plus_1 - *Vi;
			if ((u.crossproduct(v) * normal) > 1e-5) {

				// if has no vertex inside : 
				bool has_no_vertex_inside = true;
				for (int j = 0; j < n; j++) {
					if (j == prev || j == i || j == next) continue;
					myPoint3D* p = edges[j]->source->point;

					myVector3D u1 = *Vi - *Vi_minus_1;
					myVector3D v1 = *p - *Vi_minus_1;
					myVector3D u2 = *Vi_plus_1 - *Vi;
					myVector3D v2 = *p - *Vi;
					myVector3D u3 = *Vi_minus_1 - *Vi_plus_1;
					myVector3D v3 = *p - *Vi_plus_1;

					if ((u1.crossproduct(v1) * normal) >= -1e-5 &&
						(u2.crossproduct(v2) * normal) >= -1e-5 &&
						(u3.crossproduct(v3) * normal) >= -1e-5) {
						has_no_vertex_inside = false;
						break;
					}
				}

				if (has_no_vertex_inside) {
					// clip Vi+1 and Vi-1
					myHalfedge* e_prev = edges[prev];
					myHalfedge* e_curr = edges[i];

					myHalfedge* diag_in = new myHalfedge();
					myHalfedge* diag_out = new myHalfedge();
					diag_in->twin = diag_out;
					diag_out->twin = diag_in;

					diag_in->source = edges[next]->source;
					diag_out->source = edges[prev]->source;

					halfedges.push_back(diag_in);
					halfedges.push_back(diag_out);

					myFace* newFace = new myFace();
					faces.push_back(newFace);
					newFace->adjacent_halfedge = e_prev;

					e_prev->next = e_curr;   e_curr->prev = e_prev;
					e_curr->next = diag_in;  diag_in->prev = e_curr;
					diag_in->next = e_prev;  e_prev->prev = diag_in;

					e_prev->adjacent_face = newFace;
					e_curr->adjacent_face = newFace;
					diag_in->adjacent_face = newFace;

					diag_out->next = edges[next];
					diag_out->prev = edges[(prev - 1 + n) % n];
					edges[next]->prev = diag_out;
					edges[(prev - 1 + n) % n]->next = diag_out;

					diag_out->adjacent_face = f;
					f->adjacent_halfedge = diag_out;

					// remove Vi
					edges[prev] = diag_out;
					edges.erase(edges.begin() + i);

					earFound = true;
					break;
				}
			}
		}

		if (!earFound) {
			cout << "Attention : Poly concavite bloquee." << endl;
			break;
		}
	}

	if (edges.size() == 3) {
		edges[0]->next = edges[1]; edges[1]->prev = edges[0];
		edges[1]->next = edges[2]; edges[2]->prev = edges[1];
		edges[2]->next = edges[0]; edges[0]->prev = edges[2];

		edges[0]->adjacent_face = f;
		edges[1]->adjacent_face = f;
		edges[2]->adjacent_face = f;
		f->adjacent_halfedge = edges[0];
	}

	return true;
}

