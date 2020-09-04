#ifndef MESHTOOLS_H
#define MESHTOOLS_H

#include "mesh.h"
#include <QVector3D>

void subdivideCatmullClark(Mesh* inputMesh, Mesh *subdivMesh);
void generateLimitMesh(Mesh* inputMesh, Mesh *limitMesh);

QVector3D vertexPoint(HalfEdge* firstEdge, Mesh *newMesh);
QVector3D boundaryEdgePoint(HalfEdge* edge);
QVector3D normalEdgePoint(HalfEdge* edge, Mesh* mesh);
QVector3D edgePoint(HalfEdge* edge, Mesh* subdivMesh);
QVector3D facePoint(HalfEdge* firstEdge);

HalfEdge* vertOnBoundary(Vertex* currentVertex);

void splitHalfEdges(Mesh* inputMesh, Mesh* subdivMesh, unsigned int numHalfEdges, unsigned int numVertPts, unsigned int numFacePts);

#endif // MESHTOOLS_H
