#include "meshtools.h"

void subdivideCatmullClark(Mesh* inputMesh, Mesh* subdivMesh) {
  unsigned int numVerts, numHalfEdges, numFaces, sumFaceValences;
  unsigned int k, m, s, t;
  unsigned int vIndex, hIndex, fIndex;
  unsigned short n;
  HalfEdge* currentEdge;

  qDebug() << ":: Creating new Catmull-Clark mesh";

  numVerts = inputMesh->Vertices.size();
  numHalfEdges = inputMesh->HalfEdges.size();
  numFaces = inputMesh->Faces.size();

  // Reserve memory
  subdivMesh->Vertices.reserve(numFaces + numVerts + numHalfEdges/2);

  sumFaceValences = 0;
  for (k=0; k<numFaces; k++) {
    sumFaceValences += inputMesh->Faces[k].val;
  }

  subdivMesh->HalfEdges.reserve(2*numHalfEdges + 2*sumFaceValences);
  subdivMesh->Faces.reserve(sumFaceValences);

  // Create face points
  for (k=0; k<numFaces; k++) {
    n = inputMesh->Faces[k].val;
    // Coords (x,y,z), Out, Valence, Index
    subdivMesh->Vertices.append( Vertex(facePoint(inputMesh->Faces[k].side),
                                        nullptr,
                                        n,
                                        k) );
  }

  qDebug() << " * Created face points";

  vIndex = numFaces;

  // Create vertex points
  for (k=0; k<numVerts; k++) {
    n = inputMesh->Vertices[k].val;
    // Coords (x,y,z), Out, Valence, Index
    subdivMesh->Vertices.append( Vertex(vertexPoint(inputMesh->Vertices[k].out, subdivMesh),
                                        nullptr,
                                        n,
                                        vIndex) );
    vIndex++;
  }

  qDebug() << " * Created vertex points";

  // Create edge points
  for (k=0; k<numHalfEdges; k++) {
    currentEdge = &inputMesh->HalfEdges[k];

    if (k < currentEdge->twin->index) {
      m = (!currentEdge->polygon || !currentEdge->twin->polygon) ? 3 : 4;
      // Coords (x,y,z), Out, Valence, Index
      subdivMesh->Vertices.append( Vertex(edgePoint(currentEdge, subdivMesh),
                                          nullptr,
                                          m,
                                          vIndex) );
      vIndex++;
    }
  }

  qDebug() << " * Created edge points";

  // Split halfedges
  splitHalfEdges(inputMesh, subdivMesh, numHalfEdges, numVerts, numFaces);

  qDebug() << " * Split halfedges";

  hIndex = 2*numHalfEdges;
  fIndex = 0;

  // Create faces and remaining halfedges
  for (k=0; k<numFaces; k++) {
    currentEdge = inputMesh->Faces[k].side;
    n = inputMesh->Faces[k].val;

    for (m=0; m<n; m++) {

      s = currentEdge->prev->index;
      t = currentEdge->index;

      // Side, Val, Index
      subdivMesh->Faces.append(Face(nullptr,
                                    4,
                                    fIndex));

      subdivMesh->Faces[fIndex].side = &subdivMesh->HalfEdges[ 2*t ];

      // Target, Next, Prev, Twin, Poly, Index
      subdivMesh->HalfEdges.append(HalfEdge( &subdivMesh->Vertices[k],
                                             nullptr,
                                             &subdivMesh->HalfEdges[ 2*t ],
                                   nullptr,
                                   &subdivMesh->Faces[fIndex],
                                   hIndex ));

      subdivMesh->HalfEdges.append(HalfEdge( nullptr,
                                             &subdivMesh->HalfEdges[2*s+1],
                                   &subdivMesh->HalfEdges[hIndex],
                                   nullptr,
                                   &subdivMesh->Faces[fIndex],
                                   hIndex+1 ));

      subdivMesh->HalfEdges[hIndex].next = &subdivMesh->HalfEdges[hIndex+1];
      subdivMesh->HalfEdges[hIndex+1].target = subdivMesh->HalfEdges[hIndex+1].next->twin->target;

      subdivMesh->HalfEdges[2*s+1].next = &subdivMesh->HalfEdges[2*t];
      subdivMesh->HalfEdges[2*s+1].prev = &subdivMesh->HalfEdges[hIndex+1];
      subdivMesh->HalfEdges[2*s+1].polygon = &subdivMesh->Faces[fIndex];

      subdivMesh->HalfEdges[2*t].next = &subdivMesh->HalfEdges[hIndex];
      subdivMesh->HalfEdges[2*t].prev = &subdivMesh->HalfEdges[2*s+1];
      subdivMesh->HalfEdges[2*t].polygon = &subdivMesh->Faces[fIndex];

      if (m > 0) {
        // Twins
        subdivMesh->HalfEdges[hIndex+1].twin = &subdivMesh->HalfEdges[hIndex-2];
        subdivMesh->HalfEdges[hIndex-2].twin = &subdivMesh->HalfEdges[hIndex+1];
      }

      // For edge points
      subdivMesh->HalfEdges[2*t].target->out = &subdivMesh->HalfEdges[hIndex];

      hIndex += 2;
      fIndex++;
      currentEdge = currentEdge->next;
    }

    subdivMesh->HalfEdges[hIndex-2*n+1].twin = &subdivMesh->HalfEdges[hIndex-2];
    subdivMesh->HalfEdges[hIndex-2].twin = &subdivMesh->HalfEdges[hIndex-2*n+1];

    // For face points
    subdivMesh->Vertices[k].out = &subdivMesh->HalfEdges[hIndex-1];

  }

  qDebug() << " * Created faces and remaining halfedges";

  // For vertex points
  for (k=0; k<numVerts; k++) {
    subdivMesh->Vertices[numFaces + k].out = &subdivMesh->HalfEdges[ 2*inputMesh->Vertices[k].out->index ];
  }

  qDebug() << " * Completed!";
  qDebug() << "   # Vertices:" << subdivMesh->Vertices.size();
  qDebug() << "   # HalfEdges:" << subdivMesh->HalfEdges.size();
  qDebug() << "   # Faces:" << subdivMesh->Faces.size();

}

// ---

// Formula for generic CC vertex point
QVector3D normalVertexPoint(HalfEdge* firstEdge, Mesh* subdivMesh) {
  unsigned short k, n;
  QVector3D sumStarPts, sumFacePts;
  QVector3D vertexPt;
  HalfEdge* currentEdge;
  Vertex* currentVertex;

  currentVertex = firstEdge->twin->target;
  n = currentVertex->val;

  sumStarPts = QVector3D();
  sumFacePts = QVector3D();
  currentEdge = firstEdge;

  // Catmull-Clark (also supporting initial meshes containing n-gons)
  if (HalfEdge* boundaryEdge = vertOnBoundary(currentVertex)) {
    if (boundaryEdge->twin->target->val == 2) {
      // Interpolate corners
      vertexPt = boundaryEdge->twin->target->coords;
    }
    else {
      // 1/8 3/4 1/8
      vertexPt  = 1.0 * boundaryEdge->target->coords;
      vertexPt += 6.0 * boundaryEdge->twin->target->coords;
      vertexPt += 1.0 * boundaryEdge->prev->twin->target->coords;
      vertexPt /= 8.0;
    }
  }
  else
  {
    for (k=0; k<n; k++) {
      sumStarPts += currentEdge->target->coords;
      sumFacePts += subdivMesh->Vertices[currentEdge->polygon->index].coords;
      currentEdge = currentEdge->prev->twin;
    }

    vertexPt = ((n-2)*currentVertex->coords + sumStarPts/n + sumFacePts/n)/n;
  }

  return vertexPt;
}

// Formulae for all vertex points
QVector3D vertexPoint(HalfEdge* firstEdge, Mesh* subdivMesh) {
  HalfEdge* currentEdge = firstEdge;
  Vertex* vertex = firstEdge->twin->target;
  int sharpCount = 0;
  float sharpness = 0.0;
  HalfEdge* creaseEdge1 = nullptr;
  HalfEdge* creaseEdge2 = nullptr;

  for (int i = 0; i < vertex->val; i++) {
    // If we have a sharpness...
    if (currentEdge->sharpness > 0.0f) {
      sharpness = currentEdge->sharpness;
      // count the amount of sharp edges connected to this vertex
      sharpCount++;
      // save for later
      if (!creaseEdge1) {
        creaseEdge1 = currentEdge;
      }
      else if (!creaseEdge2) {
        creaseEdge2 = currentEdge;
      }
      else
      {
        break;
      }
    }
    currentEdge = currentEdge->prev->twin;
  }

  // use normal cc rules
  if (sharpCount < 2) {
    return normalVertexPoint(currentEdge, subdivMesh);
  }
  // corner rules (aka don't change at all)
  if (sharpCount > 2) {
    return vertex->coords;
  }

  // 1/8 3/4 1/8
  QVector3D vertexPt;
  vertexPt  = creaseEdge1->target->coords;
  vertexPt += creaseEdge2->target->coords;
  vertexPt += 6 * vertex->coords;
  vertexPt /= 8;
  if (sharpness < 1.0f) {
    // Decimal case; linear interpolation between this and next step
    vertexPt *= sharpness;
    vertexPt += (1-sharpness) * normalVertexPoint(currentEdge, subdivMesh);
  }
  return vertexPt;
}

// Boundary/sharp edge rules
QVector3D boundaryEdgePoint(HalfEdge* edge) {
  QVector3D EdgePt;
  EdgePt  = edge->target->coords;
  EdgePt += edge->twin->target->coords;
  EdgePt /= 2.0;
  return EdgePt;
}

// Normal cc edge rules
QVector3D normalEdgePoint(HalfEdge* edge, Mesh* mesh) {
  QVector3D EdgePt;
  EdgePt  = edge->target->coords;
  EdgePt += edge->twin->target->coords;
  EdgePt += mesh->Vertices[edge->polygon->index].coords;
  EdgePt += mesh->Vertices[edge->twin->polygon->index].coords;
  EdgePt /= 4.0;
  return EdgePt;
}

QVector3D edgePoint(HalfEdge* edge, Mesh* subdivMesh) {
  // Catmull-Clark (also supporting initial meshes containing n-gons)
  if (!edge->polygon || !edge->twin->polygon || edge->sharpness > 1.0) {
    // If we have a boundary or sharpness
    return boundaryEdgePoint(edge);
  }
  else if (edge->sharpness > 0.0f) {
    // Decimal rules, linear interpolate between this and next
    return edge->sharpness * boundaryEdgePoint(edge)
      + (1 - edge->sharpness) * normalEdgePoint(edge, subdivMesh);
  }
  else
  {
    // Normal rules
    return normalEdgePoint(edge, subdivMesh);
  }
}

QVector3D facePoint(HalfEdge* firstEdge) {
  unsigned short k, n;
  QVector<float> stencil;
  QVector3D facePt;
  HalfEdge* currentEdge;

  n = firstEdge->polygon->val;

  // Bilinear, Catmull-Clark, Dual
  stencil.clear();
  stencil.fill(1.0/n, n);

  facePt = QVector3D();
  currentEdge = firstEdge;

  for (k=0; k<n; k++) {
    // General approach
    facePt += stencil[k] * currentEdge->target->coords;
    currentEdge = currentEdge->next;
  }

  return facePt;

}

HalfEdge* vertOnBoundary(Vertex* currentVertex) {

  unsigned short n = currentVertex->val;
  int k;
  HalfEdge* currentEdge = currentVertex->out;

  for (k=0; k<n; k++) {
    if (!currentEdge->polygon) {
      return currentEdge;
    }
    currentEdge = currentEdge->prev->twin;
  }

  return nullptr;
}

// For Bilinear, Catmull-Clark and Loop
void splitHalfEdges(Mesh* inputMesh, Mesh* subdivMesh, unsigned int numHalfEdges, unsigned int numVertPts, unsigned int numFacePts) {
  unsigned int k, m;
  unsigned int vIndex;
  HalfEdge* currentEdge;

  vIndex = numFacePts + numVertPts;

  for (k=0; k<numHalfEdges; k++) {
    currentEdge = &inputMesh->HalfEdges[k];
    m = currentEdge->twin->index;

    // Target, Next, Prev, Twin, Poly, Index
    subdivMesh->HalfEdges.append(HalfEdge(nullptr,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          2*k));

    subdivMesh->HalfEdges.append(HalfEdge(nullptr,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          2*k+1));

    if (k < m) {
      subdivMesh->HalfEdges[2*k].target = &subdivMesh->Vertices[ vIndex ];
      subdivMesh->HalfEdges[2*k+1].target = &subdivMesh->Vertices[ numFacePts + currentEdge->target->index ];
      vIndex++;
    }
    else {
      subdivMesh->HalfEdges[2*k].target = subdivMesh->HalfEdges[2*m].target;
      subdivMesh->HalfEdges[2*k+1].target = &subdivMesh->Vertices[ numFacePts + currentEdge->target->index ];

      // Assign Twins
      subdivMesh->HalfEdges[2*k].twin = &subdivMesh->HalfEdges[2*m+1];
      subdivMesh->HalfEdges[2*k+1].twin = &subdivMesh->HalfEdges[2*m];
      subdivMesh->HalfEdges[2*m].twin = &subdivMesh->HalfEdges[2*k+1];
      subdivMesh->HalfEdges[2*m+1].twin = &subdivMesh->HalfEdges[2*k];

      // Boundary edges are added last when importing a mesh, so their index will always be higher than their twins.
      if (!currentEdge->polygon) {
        subdivMesh->HalfEdges[2*k].next = &subdivMesh->HalfEdges[2*k+1];
        subdivMesh->HalfEdges[2*k+1].prev = &subdivMesh->HalfEdges[2*k];

        if (currentEdge > currentEdge->next) {
          m = currentEdge->next->index;
          subdivMesh->HalfEdges[2*k+1].next = &subdivMesh->HalfEdges[2*m];
          subdivMesh->HalfEdges[2*m].prev = &subdivMesh->HalfEdges[2*k+1];
        }

        if (currentEdge > currentEdge->prev) {
          m = currentEdge->prev->index;
          subdivMesh->HalfEdges[2*k].prev = &subdivMesh->HalfEdges[2*m+1];
          subdivMesh->HalfEdges[2*m+1].next = &subdivMesh->HalfEdges[2*k];
        }
      }
    }
  }

  // Note that Next, Prev and Poly are not yet assigned at this point.

  for (int i = 0; i < inputMesh->HalfEdges.size(); i++) {
    currentEdge = &inputMesh->HalfEdges[i];
    if (currentEdge->sharpness > 0.0f) {
      subdivMesh->HalfEdges[2 * i + 1].sharpness = currentEdge->sharpness - 1;
      subdivMesh->HalfEdges[2 * i + 1].twin->sharpness = currentEdge->sharpness - 1;
    }
  }
}

QVector3D faceAverage(Face* inputFace){
    int val = inputFace->val;
    HalfEdge* currentEdge = inputFace->side;
    QVector3D avg = QVector3D(0.0, 0.0, 0.0);
    for (int i = 0; i < val; i++) {
        avg += currentEdge->target->coords;
        currentEdge = currentEdge->next;
    }
    return avg / val;
}

void generateLimitMesh(Mesh* input, Mesh* limit){
    int vertCount = input->Vertices.size();
    int halfEdgeCount = input->HalfEdges.size();
    int faceCount = input->Faces.size();

    for (int i = 0; i < input->Vertices.size(); i++) {
        Vertex* currentVertex = &input->Vertices[i];
        int val = currentVertex->val;

        Vertex limitVertex;
        limitVertex.val = val;
        limitVertex.index = i;

        HalfEdge* currentEdge = currentVertex->out;

        if (HalfEdge* boundaryEdge = vertOnBoundary(currentVertex)) {
            // magic numbers! (boundary case)
            limitVertex.coords = boundaryEdge->target->coords;
            limitVertex.coords += 4.0 * boundaryEdge->prev->target->coords;
            limitVertex.coords += boundaryEdge->prev->twin->target->coords;
            limitVertex.coords /= 6.0;
        }
        else
        {
            QVector3D coords = currentVertex->coords;
            // magic numbers! (normal case)
            limitVertex.coords = (val - 3.0f) * coords / (val + 5.0f);
            float c = 4.0f / (val * (val + 5.0f));
            for (int k = 0; k < val; ++k){
                limitVertex.coords += 0.5f * c * (coords + currentEdge->target->coords);
                limitVertex.coords += c * faceAverage(currentEdge->polygon);
                currentEdge = currentEdge->prev->twin;
            }
        }
        limit->Vertices.append(limitVertex);
    }

    // copy the data
    for (int i = 0; i < halfEdgeCount; i++) {
        limit->HalfEdges.append(input->HalfEdges[i]);
    }

    for (int i = 0; i < vertCount; i++) {
        limit->Vertices[i].out = &limit->HalfEdges[input->Vertices[i].out->index];
    }

    for (int i = 0; i < halfEdgeCount; i++) {
        limit->HalfEdges[i].target = &limit->Vertices[input->HalfEdges[i].target->index];
        limit->HalfEdges[i].next = &limit->HalfEdges[input->HalfEdges[i].next->index];
        limit->HalfEdges[i].prev = &limit->HalfEdges[input->HalfEdges[i].prev->index];
        limit->HalfEdges[i].twin = &limit->HalfEdges[input->HalfEdges[i].twin->index];
    }

    for (int i = 0; i < faceCount; i++) {
        limit->Faces.append(input->Faces[i]);
        limit->Faces[i].side = &limit->HalfEdges[input->Faces[i].side->index];
    }
}
