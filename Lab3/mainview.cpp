#include "mainview.h"

MainView::MainView(QWidget *Parent) : QOpenGLWidget(Parent) {
  qDebug() << "✓✓ MainView constructor";

  modelLoaded = false;
  wireframeMode = true;

  rotAngle = 0.0;
  FoV = 60.0;
}

MainView::~MainView() {
  qDebug() << "✗✗ MainView destructor";

  glDeleteBuffers(1, &meshCoordsBO);
  glDeleteBuffers(1, &meshNormalsBO);
  glDeleteBuffers(1, &meshIndexBO);
  glDeleteVertexArrays(1, &meshVAO);

  glDeleteBuffers(1, &quadCoordsBO);
  glDeleteBuffers(1, &quadIndexBO);
  glDeleteVertexArrays(1, &quadVAO);

  debugLogger->stopLogging();

  delete mainShaderProg;
  delete tesselationShaderProg;
}

// ---

void MainView::createShaderPrograms() {
  qDebug() << ".. createShaderPrograms";

  // Main shader linkage
  mainShaderProg = new QOpenGLShaderProgram();
  mainShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertshader.glsl");
  mainShaderProg->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragshader.glsl");

  mainShaderProg->link();

  // Shader for tessellating regular quads
  tesselationShaderProg = new QOpenGLShaderProgram();
  tesselationShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/tesselation-vertshader.glsl");
  tesselationShaderProg->addShaderFromSourceFile(QOpenGLShader::TessellationControl, ":/shaders/tcs.glsl");
  tesselationShaderProg->addShaderFromSourceFile(QOpenGLShader::TessellationEvaluation, ":/shaders/tes.glsl");
  tesselationShaderProg->addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/tesselation-geomshader.glsl");
  tesselationShaderProg->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/tesselation-fragshader.glsl");

  tesselationShaderProg->link();

  // Uniforms
  uniModelViewMatrix = glGetUniformLocation(mainShaderProg->programId(), "modelviewmatrix");
  uniProjectionMatrix = glGetUniformLocation(mainShaderProg->programId(), "projectionmatrix");
  uniNormalMatrix = glGetUniformLocation(mainShaderProg->programId(), "normalmatrix");

  tessUniModelViewMatrix = glGetUniformLocation(tesselationShaderProg->programId(), "modelviewmatrix");
  tessUniProjectionMatrix = glGetUniformLocation(tesselationShaderProg->programId(), "projectionmatrix");
  tessUniNormalMatrix = glGetUniformLocation(tesselationShaderProg->programId(), "normalmatrix");
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

  // Quad Mesh

  glGenVertexArrays(1, &quadVAO);
  glBindVertexArray(quadVAO);

  glGenBuffers(1, &quadCoordsBO);
  glBindBuffer(GL_ARRAY_BUFFER, quadCoordsBO);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &quadIndexBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIndexBO);

  glBindVertexArray(0);
}

// Used to build the quad mesh
void MainView::buildQuadMesh()
{
    // Here we find and index the regular quads from the currently displayed mesh
    HalfEdge* currentEdge, *startEdge;
    quadCoords.clear();
    quadCoords.squeeze();

    quadIndices.clear();
    quadIndices.squeeze();

    unsigned int k, n;
    bool isRegular;

    Mesh *currentMesh = showLimit ? limitMesh : &Meshes[currentMeshIndex]; // Load either (subdivided) mesh or limit mesh
    unsigned int index = 0;

    for (k=0; k<(GLuint)currentMesh->Faces.size(); k++) {
        n = currentMesh->Faces[k].val;
         // We are looking at a quad
        if (n == 4) {
            isRegular = true;
            startEdge = currentMesh->Faces[k].side;
            // Check the valencies of the vertices of the face
            for (int j = 0; j < 4; ++j) {
                if (startEdge->target->val != 4) {
                    isRegular = false;
                    break;
                }
                startEdge = startEdge->next;
            }

            if (isRegular) {
                // Select the one ring neighborhood of the regular face
                startEdge = currentMesh->Faces[k].side->twin->next->twin->prev;
                for (int p = 0; p < 4; ++p) {
                    currentEdge = startEdge;
                    quadCoords.append(currentEdge->twin->target->coords);
                    quadIndices.append(index);
                    index++;

                    for (int h = 0; h < 3; ++h) {
                        quadCoords.append(currentEdge->target->coords);
                        currentEdge = currentEdge->next->twin->next;
                        quadIndices.append(index);
                        index++;
                    }
                    startEdge = startEdge->next->next->twin;
                }
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, quadCoordsBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*quadCoords.size(), quadCoords.data(), GL_DYNAMIC_DRAW);

    qDebug() << " → Updated quadCoordsBO";

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIndexBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*quadIndices.size(), quadIndices.data(), GL_DYNAMIC_DRAW);

}

void MainView::updateMeshBuffers(Mesh* currentMesh) {

  qDebug() << ".. updateBuffers";

  unsigned int k;
  unsigned short n, m;
  HalfEdge* currentEdge;

  vertexCoords.clear();
  vertexCoords.reserve(currentMesh->Vertices.size());

  for (k=0; k<currentMesh->Vertices.size(); k++) {
    vertexCoords.append(currentMesh->Vertices[k].coords);
  }

  vertexNormals.clear();
  vertexNormals.reserve(currentMesh->Vertices.size());

  for (k=0; k<(GLuint)currentMesh->Faces.size(); k++) {
    currentMesh->setFaceNormal(&currentMesh->Faces[k]);
  }

  for (k=0; k<currentMesh->Vertices.size(); k++) {
    vertexNormals.append( currentMesh->computeVertexNormal(&currentMesh->Vertices[k]) );
  }

  polyIndices.clear();
  polyIndices.reserve(currentMesh->HalfEdges.size() + currentMesh->Faces.size());

  for (k=0; k<(GLuint)currentMesh->Faces.size(); k++) {
    n = currentMesh->Faces[k].val;
    currentEdge = currentMesh->Faces[k].side;
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


    if (showTesselation){
        tesselationShaderProg->bind();
        glUniformMatrix4fv(tessUniModelViewMatrix, 1, false, modelViewMatrix.data());
        glUniformMatrix4fv(tessUniProjectionMatrix, 1, false, projectionMatrix.data());
        glUniformMatrix3fv(tessUniNormalMatrix, 1, false, normalMatrix.data());
        tesselationShaderProg->release();
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

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );


    if (showTesselation)
    {
        glBindVertexArray(quadVAO);
        tesselationShaderProg->bind();
        glDrawElements(GL_PATCHES, quadCoords.size(), GL_UNSIGNED_INT, 0);
        tesselationShaderProg->release();
    }
    else
    {
        glBindVertexArray(meshVAO);
        mainShaderProg->bind();
        if (wireframeMode) {
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
            glDrawElements(GL_LINE_LOOP, meshIBOSize, GL_UNSIGNED_INT, 0);
        }
        else {
            glDrawElements(GL_TRIANGLE_FAN, meshIBOSize, GL_UNSIGNED_INT, 0);
        }
        mainShaderProg->release();
    }

  }
}

// ---

void MainView::renderMesh() {

  glBindVertexArray(meshVAO);

  if (wireframeMode) {
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glDrawElements(GL_LINE_LOOP, meshIBOSize, GL_UNSIGNED_INT, 0);
  }
  else {
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glDrawElements(GL_TRIANGLE_FAN, meshIBOSize, GL_UNSIGNED_INT, 0);
  }

  glBindVertexArray(0);

}

// ---

void MainView::mousePressEvent(QMouseEvent* event) {
  setFocus();
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
