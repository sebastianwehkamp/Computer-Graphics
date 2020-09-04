#include "mainview.h"
#include "math.h"

MainView::MainView(QWidget *parent) : QOpenGLWidget(parent) {
  qDebug() << "✓✓ MainView constructor";
  showCurveColour = false;
  selectedPt = -1;
  steps = 0;
}

MainView::~MainView() {
  qDebug() << "✗✗ MainView destructor";

  clearArrays();
  clearCurve();

  // Delete vertex arrays
  glDeleteBuffers(1, &netCoordsBO);
  glDeleteVertexArrays(1, &netVAO);

  glDeleteBuffers(1, &curveCoordsBO);
  glDeleteVertexArrays(1, &curveVAO);

  // Delete colour arrays
  glDeleteBuffers(1, &curveColourBO);
  glDeleteBuffers(1, &netColourBO);

  // delete shader programs
  delete mainShaderProg;
  delete curveShaderProg;

  debugLogger->stopLogging();
}

// ---

void MainView::createShaderPrograms() {

    // Main shader program
    mainShaderProg = new QOpenGLShaderProgram();
    mainShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertshader.glsl");
    mainShaderProg->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragshader.glsl");

    mainShaderProg->link();

    // Shader program which uses a geometry shader to shade the curvature
    curveShaderProg = new QOpenGLShaderProgram();
    curveShaderProg->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vertshader.glsl");
    curveShaderProg->addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/geometry.glsl");
    curveShaderProg->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fragshader.glsl");
}

void MainView::createBuffers() {

    // Net points
    glGenVertexArrays(1, &netVAO);
    glBindVertexArray(netVAO);

    glGenBuffers(1, &netCoordsBO);
    glBindBuffer(GL_ARRAY_BUFFER, netCoordsBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &netColourBO);
    glBindBuffer(GL_ARRAY_BUFFER, netColourBO);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Curve points
    glGenVertexArrays(1, &curveVAO);
    glBindVertexArray(curveVAO);

    glGenBuffers(1, &curveCoordsBO);
    glBindBuffer(GL_ARRAY_BUFFER, curveCoordsBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // Colours
    glGenBuffers(1, &curveColourBO);
    glBindBuffer(GL_ARRAY_BUFFER, curveColourBO);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);
}

// Used to reload the buffers when a setting is changed
void MainView::reload(){
    update();
    updateBuffers();
    selectedPt = -1; // Deselect point when options are (de)selected etc. Just personal preference
}

// Function which creates a matrix using both stencils
Eigen::MatrixXf MainView::getMatrix(int colCount){
    // Define the matrix and initialise with zeros
    Eigen::MatrixXf M;
    int rowCount = 2*colCount - subdivMask.count() + 2;
    M.setZero(rowCount, colCount);
    QVector<short int> currentStencil = secondStencil;

    // Fill the matrix
    int offset = 0;
    for(int row=0; row<M.rows(); row++){
        for(int col=0;col<currentStencil.count();col++){
            M(row,col+offset) = currentStencil[col];
        }
        if (row%2 == 1){
            currentStencil = firstStencil;
            offset++;
        } else {
            currentStencil = secondStencil;
        }
    }
    return M;
}

// Calcuate the subdvision using matrix multiplication. This is why eigne is used since it includes Matrix
// Multiplication
QVector<QVector2D> MainView::calculateSubdivision(QVector<QVector2D> points){
      // Copy control point to a new vector
      QVector<QVector2D> orignal;

      orignal.reserve(points.size());
      for(int i=0; i<points.size(); i++){
        orignal.append(points.at(i));
      }
      for(int s=0; s<steps; s++){
          // Create P
          Eigen::VectorXf Px(orignal.count()), Py(orignal.count());
          for(int i = 0; i<orignal.count(); i++){
              Px(i) = orignal.at(i).x();
              Py(i) = orignal.at(i).y();
          }

          // Create matrix
          Eigen::MatrixXf M = getMatrix(orignal.size());

          // Multiply vector P with matrix M
          Eigen::VectorXf newX(M.rows()), newY(M.rows());
          newX = M*Px/normalizeValue;
          newY = M*Py/normalizeValue;

          // insert points in a new vector
          orignal.clear();
          orignal.reserve(newX.size());
          for(int i=0; i<newX.size(); i++){
            orignal.append(QVector2D(newX(i),newY(i)));
          }
      }
      return orignal;
}

void MainView::updateBuffers() {

    clearCurve();

    curveCoords = calculateSubdivision(netCoords);

    for (int i = 0; i < curveCoords.size(); i++){
        curveColours.append(QVector3D(1.0, 1.0, 1.0));
    }

    // Bind all the buffers
    glBindBuffer(GL_ARRAY_BUFFER, netCoordsBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QVector2D)*netCoords.size(), netCoords.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, netColourBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*netColours.size(), netColours.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, curveCoordsBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QVector2D)*curveCoords.size(), curveCoords.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, curveColourBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QVector3D)*curveColours.size(), curveColours.data(), GL_DYNAMIC_DRAW);

    update();

}

void MainView::updateUniforms() {

    //  glUniform...();

    updateUniformsRequired = false;
}

void MainView::clearArrays() {

  // As of Qt 5.6, clear() does not release the memory anymore. Use e.g. squeeze()
    netCoords.clear();
    netCoords.squeeze();
}

// Clears the curve arrays
void MainView::clearCurve() {
    curveCoords.clear();
    curveCoords.squeeze();
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

    // If the application crashes here, try setting "MESA_GL_VERSION_OVERRIDE = 4.1"
    // and "MESA_GLSL_VERSION_OVERRIDE = 410" in Projects (left panel) -> Build Environment

    QString glVersion;
    glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    qDebug() << ":: Using OpenGL" << qPrintable(glVersion);

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    // Default is GL_LESS
    glDepthFunc(GL_LEQUAL);

    // ---

    createShaderPrograms();
    createBuffers();

    presetNet(0);
  }

  void MainView::paintGL() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (updateUniformsRequired) {
    updateUniforms();
    }

    // Only paint if the net should be shown.
    if (showNet) {
        mainShaderProg->bind();
        glBindVertexArray(netVAO);

        // Draw control net
        glDrawArrays(GL_LINE_STRIP, 0, netCoords.size());
        glPointSize(8.0);
        glDrawArrays(GL_POINTS, 0, netCoords.size());

        // Highlight selected control point
        if (selectedPt > -1) {
          glPointSize(12.0);
          glDrawArrays(GL_POINTS, selectedPt, 1);
        }

        glBindVertexArray(0);
        mainShaderProg->release();
    }

    // If steps>0 is needed otherwise the drawing goes wrong for the first step.
    if(steps>0){
        // Show the curved points only if checked
        if (showCurvePts) {
            // Shade correctly
            QOpenGLShaderProgram* shaderProg = showCurveColour ? curveShaderProg : mainShaderProg;
            shaderProg->bind();

            // Draw correctly
            GLuint mode = showCurveColour ? GL_LINE_STRIP_ADJACENCY : GL_LINE_STRIP;

            // Draw the actual curve
            glBindVertexArray(curveVAO);
            glDrawArrays(mode, 0, curveCoords.size());

            shaderProg->release();
        }
    }
}

// ---

void MainView::setSteps(int value){ // set subdivision steps
    steps = value;
}


void MainView::presetNet(unsigned short preset) {

  selectedPt = -1;
  clearArrays();

  switch (preset) {
  case 0:
    // 'Pentagon'
    netCoords.reserve(5);
    netCoords.append(QVector2D(-0.25, -0.5));
    netCoords.append(QVector2D(-0.72, 0.0));
    netCoords.append(QVector2D(-0.25, 0.73));
    netCoords.append(QVector2D(0.79, 0.5));
    netCoords.append(QVector2D(0.5, -0.73));
    break;
  case 1:
    // 'Basis'
    netCoords.reserve(9);
    netCoords.append(QVector2D(-1.0, -0.25));
    netCoords.append(QVector2D(-0.75, -0.25));
    netCoords.append(QVector2D(-0.5, -0.25));
    netCoords.append(QVector2D(-0.25, -0.25));
    netCoords.append(QVector2D(0.0, 0.50));
    netCoords.append(QVector2D(0.25, -0.25));
    netCoords.append(QVector2D(0.5, -0.25));
    netCoords.append(QVector2D(0.75, -0.25));
    netCoords.append(QVector2D(1.0, -0.25));
    break;
  }
  netColours.clear();
  netColours.squeeze();
  for (int i = 0; i < netCoords.size(); i++){
      netColours.append(QVector3D(1.0, 0.0, 0.0));
  }
  updateBuffers();

}

void MainView::setMask(QString stringMask) {

  subdivMask.clear();

  QString trimmedMask;
  trimmedMask = stringMask.trimmed();

  // Convert to sequence of integers
  QTextStream intSeq(&trimmedMask);
  while (!intSeq.atEnd()) {
    int k;
    intSeq >> k;
    subdivMask.append(k);
  }

  // Stencils represent affine combinations (i.e. they should sum to unity)
  normalizeValue = 0;

  firstStencil.clear();
  secondStencil.clear();

  for (int k=0; k<subdivMask.size(); k++) {
    if (k % 2) {
      normalizeValue += subdivMask[k];
      firstStencil.append(subdivMask[k]);
    }
    else {
      secondStencil.append(subdivMask[k]);
    }
  }

  qDebug() << ":: Extracted stencils" << firstStencil << "and" << secondStencil;
}

// ---

void MainView::mousePressEvent(QMouseEvent *event) {

  // In order to allow keyPressEvents:
  setFocus();

  float xRatio, yRatio, xScene, yScene;

  xRatio = (float)event->x() / width();
  yRatio = (float)event->y() / height();

  // By default, the drawing canvas is the square [-1,1]^2:
  xScene = (1-xRatio)*-1 + xRatio*1;
  // Note that the origin of the canvas is in the top left corner (not the lower left).
  yScene = yRatio*-1 + (1-yRatio)*1;

  switch (event->buttons()) {
  case Qt::LeftButton:
    if (selectedPt > -1) {
      // De-select control point
      selectedPt = -1;
      setMouseTracking(false);
      update();
    }
    else {
      // Add new control point
      netCoords.append(QVector2D(xScene, yScene));
      netColours.append(QVector3D(1.0,0,0));
      updateBuffers();
    }
    break;
  case Qt::RightButton:
    // Select control point
    selectedPt = findClosest(xScene, yScene);
    update();
    break;
  }

}

void MainView::mouseMoveEvent(QMouseEvent *event) {

  if (selectedPt > -1) {
    float xRatio, yRatio, xScene, yScene;

    xRatio = (float)event->x() / width();
    yRatio = (float)event->y() / height();

    xScene = (1-xRatio)*-1 + xRatio*1;
    yScene = yRatio*-1 + (1-yRatio)*1;

    // Update position of the control point
    netCoords[selectedPt] = QVector2D(xScene, yScene);
    updateBuffers();
  }

}

void MainView::keyPressEvent(QKeyEvent *event) {

  // Only works when the widget has focus!

  switch(event->key()) {
  case 'G':
    if (selectedPt > -1) {
      // Grab selected control point
      setMouseTracking(true);
    }
    break;
  case 'X':
    if (selectedPt > -1) {
      // Remove selected control point
      netCoords.remove(selectedPt);
      selectedPt = -1;
      updateBuffers();
    }
    break;
  }

}

short int MainView::findClosest(float x, float y) {

  short int ptIndex = 0;
  float currentDist, minDist = 4;

  for (int k=0; k<netCoords.size(); k++) {
    currentDist = pow((netCoords[k].x()-x),2) + pow((netCoords[k].y()-y),2);
    if (currentDist < minDist) {
      minDist = currentDist;
      ptIndex = k;
    }
  }

  return ptIndex;

}

// ---

void MainView::onMessageLogged( QOpenGLDebugMessage Message ) {
  qDebug() << " → Log:" << Message;
}
