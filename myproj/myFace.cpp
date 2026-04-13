#include "myFace.h"
#include "myVector3D.h"
#include "myHalfedge.h"
#include "myVertex.h"
#include <GL/glew.h>

myFace::myFace(void)
{
	adjacent_halfedge = NULL;
	normal = new myVector3D(1.0, 1.0, 1.0);
}

myFace::~myFace(void)
{
	if (normal) delete normal;
}

void myFace::computeNormal()
{
	if (adjacent_halfedge == NULL) return;
	
	myPoint3D *p1 = adjacent_halfedge->source->point;
	myPoint3D *p2 = adjacent_halfedge->next->source->point;
	myPoint3D *p3 = adjacent_halfedge->next->next->source->point;

	myVector3D v1 = *p2 - *p1;
	myVector3D v2 = *p3 - *p1;
	
	normal->crossproduct(v1, v2);
	normal->normalize();
}
