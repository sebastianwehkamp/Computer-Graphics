#include "mainview.h"

MainView::MainView(QWidget *Parent) : QOpenGLWidget(Parent) {
  qDebug() << "✓✓ MainView constructor";

  modelLoaded = false;
  wireframeMode = true;
  limitMode = false;

  rotAngle = 0.0;
  FoV = 60.0;
}

MainView::~MainView() {
  qDebug() << "✗✗ MainView destructor";

  glDeleteBuffers(1, &meshCoordsBO);
  glDeleteBuffers(1, &meshNormalsBO);
  glDeleteBuffers(1, &meshIndexBO);
  glDeleteVertexArrays(1, &meshVAO);

  glDeleteBuffers(1, &controlCoordsBO);
  glDeleteBuffers(1, &controlIndexBO);
  glDeleteVertexArrays(1, &controlVAO);

  glDeleteBuffers(1, &selectCoordsBO);
  glDeleteBuffers(1, &selectIndexBO);
  glDeleteVertexArrays(1, &selectVAO);


  debugLogger->stopLogging();

  delete mainShaderProg;
  delete controlShaderProg;
}

// ---

void MainView::createShaderPrograms() {
  qDebug() << ".. createShaderPrograms";

  mainShaderProg = new QOpenGLShaderProgram();
  mainShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertshader.glsl");
  mainShaderProg->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragshader.glsl");

  mainShaderProg->link();

  uniModelViewMatrix = glGetUniformLocation(mainShaderProg->programId(), "modelviewmatrix");
  uniProjectionMatrix = glGetUniformLocation(mainShaderProg->programId(), "projectionmatrix");
  uniNormalMatrix = glGetUniformLocation(mainShaderProg->programId(), "normalmatrix");

  //Control net
  controlShaderProg = new QOpenGLShaderProgram();
  controlShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/control/vertshader.glsl");
  controlShaderProg->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/control/fragshader.glsl");

  controlShaderProg->link();

  // Control net uniforms
  controlUniModelViewMatrix = glGetUniformLocation(controlShaderProg->programId(), "modelviewmatrix");
  controlUniProjectionMatrix = glGetUniformLocation(controlShaderProg->programId(), "projectionmatrix");

}

void MainView::createBuffers() {

  qDebug() << ".. createBuffers";

  glGenVertexArrays(1, &meshVAO);
  glBindVertexArray(meshVAO);

  glGenBuffers(1, &meshCoordsBO);
  glBindBuffer(GL_ARRAY_BUFFER, meshCoordsBO);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &meshNormalsBO);
  glBindBuffer(GL_ARRAY_BUFFER, meshNormalsBO);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &meshIndexBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO);

  glBindVertexArray(0);

  // Control net

  glGenVertexArrays(1, &controlVAO);
  glBindVertexArray(controlVAO);

  glGenBuffers(1, &controlCoordsBO);
  glBindBuffer(GL_ARRAY_BUFFER, controlCoordsBO);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &controlColourBO);
  glBindBuffer(GL_ARRAY_BUFFER, controlColourBO);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &controlIndexBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, controlIndexBO);

  // Selected lines

  glGenVertexArrays(1, &selectVAO);
  glBindVertexArray(selectVAO);

  glGenBuffers(1, &selectCoordsBO);
  glBindBuffer(GL_ARRAY_BUFFER, selectCoordsBO);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &selectColourBO);
  glBindBuffer(GL_ARRAY_BUFFER, selectColourBO);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &selectIndexBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, selectIndexBO);


  glBindVertexArray(0);

}

void MainView::updateMeshBuffers() {

  qDebug() << ".. updateBuffers";

  unsigned int k;
  unsigned short n, m;

  HalfEdge* currentEdge;
  Mesh* displayMesh;

  if (limitMode) {
    displayMesh = limitMesh;
  } else {
    displayMesh = currentMesh;
  }

  // If the control net should be shown then we want to render it and allow for selecting edges
  if (showControlNet){
      qDebug() << "Control Net";
      controlCoords.clear();
      controlColours.clear();
      controlCoords.reserve(originalMesh->Vertices.size());
      controlColours.reserve(originalMesh->Vertices.size());
      selectCoords.clear();
      selectColours.clear();
      selectCoords.reserve(originalMesh->Vertices.size());
      selectColours.reserve(originalMesh->Vertices.size());

      // Fill in the the control/select coords and colours
      for (k = 0; k < originalMesh->Vertices.size(); k++) {
        controlCoords.append(originalMesh->Vertices[k].coords);
        controlColours.append(QVector3D(0.6, 0.8, 0.0));
        selectCoords.append(originalMesh->Vertices[k].coords);
        selectColours.append(QVector3D(1.0, 0.0, 0.0));
      }

      controlIndices.clear();
      controlIndices.reserve(originalMesh->HalfEdges.size() + originalMesh->Faces.size());
      selectIndices.clear();

      //
      for (k = 0; k < originalMesh->Faces.size(); k++) {
        n = originalMesh->Faces[k].val;
        currentEdge = originalMesh->Faces[k].side;
        for (m=0; m<n; m++) {
          if (currentEdge->index < currentEdge->twin->index){
              controlIndices.append(currentEdge->twin->target->index);
              controlIndices.append(currentEdge->target->index);
          }
          currentEdge = currentEdge->next;
        }
      }

      for (k = 0; k < originalMesh->HalfEdges.size(); k++) {
        currentEdge = &originalMesh->HalfEdges[k];
        if (currentEdge->selected) {
          selectIndices.append(currentEdge->target->index);
          selectIndices.append(currentEdge->twin->target->index);
        }
      }

      // Set the control and select buffers
      glBindBuffer(GL_ARRAY_BUFFER, controlCoordsBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*controlCoords.size(), controlCoords.data(), GL_DYNAMIC_DRAW);

      glBindBuffer(GL_ARRAY_BUFFER, controlColourBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*controlColours.size(), controlColours.data(), GL_DYNAMIC_DRAW);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, controlIndexBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*controlIndices.size(), controlIndices.data(), GL_DYNAMIC_DRAW);

      glBindBuffer(GL_ARRAY_BUFFER, selectCoordsBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*selectCoords.size(), selectCoords.data(), GL_DYNAMIC_DRAW);

      glBindBuffer(GL_ARRAY_BUFFER, selectColourBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*selectColours.size(), selectColours.data(), GL_DYNAMIC_DRAW);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, selectIndexBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*selectIndices.size(), selectIndices.data(), GL_DYNAMIC_DRAW);

      qDebug() << "Control net buffers set";
  }

  vertexCoords.clear();
  vertexCoords.reserve(displayMesh->Vertices.size());

  for (k=0; k<displayMesh->Vertices.size(); k++) {
    vertexCoords.append(displayMesh->Vertices[k].coords);
  }

  vertexNormals.clear();
  vertexNormals.reserve(displayMesh->Vertices.size());

  for (k = 0; k < displayMesh->Faces.size(); k++) {
    displayMesh->setFaceNormal(&displayMesh->Faces[k]);
  }

  for (k = 0; k < displayMesh->Vertices.size(); k++) {
    vertexNormals.append( displayMesh->computeVertexNormal(&displayMesh->Vertices[k]) );
  }

  polyIndices.clear();
  polyIndices.reserve(displayMesh->HalfEdges.size() + displayMesh->Faces.size());

  for (k = 0; k < displayMesh->Faces.size(); k++) {
    n = displayMesh->Faces[k].val;
    currentEdge = displayMesh->Faces[k].side;
    for (m=0; m<n; m++) {
      polyIndices.append(currentEdge->target->index);
      currentEdge = currentEdge->next;
    }
    polyIndices.append(maxInt);
  }

  // ---

  glBindBuffer(GL_ARRAY_BUFFER, meshCoordsBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*vertexCoords.size(), vertexCoords.data(), GL_DYNAMIC_DRAW);

  qDebug() << " → Updated meshCoordsBO";

  glBindBuffer(GL_ARRAY_BUFFER, meshNormalsBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*vertexNormals.size(), vertexNormals.data(), GL_DYNAMIC_DRAW);

  qDebug() << " → Updated meshNormalsBO";

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIndexBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*polyIndices.size(), polyIndices.data(), GL_DYNAMIC_DRAW);

  qDebug() << " → Updated meshIndexBO";

  meshIBOSize = polyIndices.size();

  update();

}

void MainView::updateMatrices() {

  modelViewMatrix.setToIdentity();
  modelViewMatrix.translate(QVector3D(0.0, 0.0, -3.0));
  modelViewMatrix.scale(QVector3D(1.0, 1.0, 1.0));
  modelViewMatrix.rotate(rotAngle, QVector3D(0.0, 1.0, 0.0));

  projectionMatrix.setToIdentity();
  projectionMatrix.perspective(FoV, dispRatio, 0.2, 4.0);

  normalMatrix = modelViewMatrix.normalMatrix();

  uniformUpdateRequired = true;
  update();

}


void MainView::updateUniforms() {

  mainShaderProg->bind();
  glUniformMatrix4fv(uniModelViewMatrix, 1, false, modelViewMatrix.data());
  glUniformMatrix4fv(uniProjectionMatrix, 1, false, projectionMatrix.data());
  glUniformMatrix3fv(uniNormalMatrix, 1, false, normalMatrix.data());
  mainShaderProg->release();

  if (showControlNet){
      controlShaderProg->bind();
      glUniformMatrix4fv(controlUniModelViewMatrix, 1, false, modelViewMatrix.data());
      glUniformMatrix4fv(controlUniProjectionMatrix, 1, false, projectionMatrix.data());
      controlShaderProg->release();
  }
}

void MainView::updateSharpness() {
  for (int i = 0; i < originalMesh->HalfEdges.size(); i++) {
    HalfEdge* currentEdge = &originalMesh->HalfEdges[i];
    if (currentEdge->selected) {
      currentEdge->sharpness = sharpness;
    }
  }
}

// ---

void MainView::initializeGL() {

  initializeOpenGLFunctions();
  qDebug() << ":: OpenGL initialized";

  debugLogger = new QOpenGLDebugLogger();
  connect( debugLogger, SIGNAL( messageLogged( QOpenGLDebugMessage ) ), this, SLOT( onMessageLogged( QOpenGLDebugMessage ) ), Qt::DirectConnection );

  if ( debugLogger->initialize() ) {
    qDebug() << ":: Logging initialized";
    debugLogger->startLogging( QOpenGLDebugLogger::SynchronousLogging );
    debugLogger->enableMessages();
  }

  QString glVersion;
  glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  qDebug() << ":: Using OpenGL" << qPrintable(glVersion);

  // Enable depth buffer
  glEnable(GL_DEPTH_TEST);
  // Default is GL_LESS
  glDepthFunc(GL_LEQUAL);

  glEnable(GL_PRIMITIVE_RESTART);
  maxInt = ((unsigned int) -1);
  glPrimitiveRestartIndex(maxInt);

  // ---

  createShaderPrograms();
  createBuffers();

  // ---

  glPatchParameteri(GL_PATCH_VERTICES, 16);

  updateMatrices();
}

void MainView::resizeGL(int newWidth, int newHeight) {

  qDebug() << ".. resizeGL";

  dispRatio = (float)newWidth/newHeight;
  updateMatrices();

}

void MainView::paintGL() {
  if (modelLoaded) {

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateUniforms();

    glBindVertexArray(meshVAO);
    mainShaderProg->bind();
    if (wireframeMode) {
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      glDrawElements(GL_LINE_LOOP, meshIBOSize, GL_UNSIGNED_INT, 0);
    } else {
      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
      glDrawElements(GL_TRIANGLE_FAN, meshIBOSize, GL_UNSIGNED_INT, 0);
    }
    mainShaderProg->release();

    if (showControlNet)
    {
        controlShaderProg->bind();
        glBindVertexArray(controlVAO);
        glDrawElements(GL_LINES, controlIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(selectVAO);
        glDrawElements(GL_LINES, selectIndices.size(), GL_UNSIGNED_INT, 0);
        controlShaderProg->release();
    }
  }
}

// On a left mouse button click we cast a ray and compute the closest edge. This edge is marked as selected
void MainView::mousePressEvent(QMouseEvent* event) {
  setFocus();
  float xRatio, yRatio, xScene, yScene;
  xRatio = (float)event->x() / width();
  yRatio = (float)event->y() / height();
  xScene = 2 * xRatio - 1;
  yScene = 1 - 2 * yRatio;

  // If left mouse button is pressed we cast a ray
  if (event->button() == Qt::LeftButton) {
    QVector4D rayDirection = projectionMatrix.inverted() * QVector4D(xScene, yScene, -1, -1);
    rayDirection[2] = -1;
    rayDirection[3] = 0;
    QMatrix4x4 modelView = modelViewMatrix.inverted();
    QVector4D ray = modelView * rayDirection;
    QVector4D lastCol = modelView.column(3);

    float dist, minDist = 1000;
    int selectedIndex = 0;

    QVector3D n, n2, p1, d1;
    QVector3D d2 = QVector3D(ray[0], ray[1], ray[2]).normalized();
    QVector3D p2 = QVector3D(lastCol[0], lastCol[1], lastCol[2]);

    float linePercent;
    HalfEdge* currentEdge;

    // Compute the closest edge
    for (int i = 0; i < originalMesh->HalfEdges.size(); i++) {
      currentEdge = &originalMesh->HalfEdges[i];
      d1 = currentEdge->twin->target->coords - currentEdge->target->coords;

      n = QVector3D::crossProduct(d2, d1).normalized();

      n2 = QVector3D::crossProduct(n, d2);

      p1 = currentEdge->target->coords;

      linePercent = QVector3D::dotProduct((p2 - p1), n2) / QVector3D::dotProduct(d1, n2);
      if (linePercent < 0 || linePercent > 1) {
        dist = 1000;
      } else {
        dist = QVector3D::dotProduct(n, (p2 - p1));
        dist = dist < 0 ? -dist : dist;
        if (dist < minDist) {
          minDist = dist;
          selectedIndex = i;
        }
      }
    }

    // Flip selected of the edge to allow for deselection
    originalMesh->HalfEdges[selectedIndex].selected = !originalMesh->HalfEdges[selectedIndex].selected;
    originalMesh->HalfEdges[selectedIndex].twin->selected = !originalMesh->HalfEdges[selectedIndex].twin->selected;

    // Set the sharpness
    originalMesh->HalfEdges[selectedIndex].sharpness = sharpness;
    originalMesh->HalfEdges[selectedIndex].twin->sharpness = sharpness;
    updateMeshBuffers();
  }
}

void MainView::wheelEvent(QWheelEvent* event) {
  FoV -= event->delta() / 60.0;
  updateMatrices();
}

void MainView::keyPressEvent(QKeyEvent* event) {
  switch(event->key()) {
  case 'Z':
    wireframeMode = !wireframeMode;
    update();
    break;
  }
}

// ---

void MainView::onMessageLogged( QOpenGLDebugMessage Message ) {
  qDebug() << " → Log:" << Message;
}
