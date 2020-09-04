#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLDebugLogger>

#include <QOpenGLShaderProgram>

#include <QMouseEvent>
#include "mesh.h"

class MainView : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core {

  Q_OBJECT

public:
  MainView(QWidget *Parent = nullptr);
  ~MainView();

  size_t currentMeshIndex = 0;
  bool modelLoaded;
  bool wireframeMode;

  float FoV;
  float dispRatio;
  float rotAngle;

  bool uniformUpdateRequired;

  void updateMatrices();
  void updateUniforms();
  void updateMeshBuffers(Mesh* currentMesh);

  //Used for limit and tesselation
  bool showLimit = false;
  QVector<Mesh> Meshes;
  Mesh *limitMesh;
  void buildQuadMesh();
  bool showTesselation = false;

protected:
  void initializeGL();
  void resizeGL(int newWidth, int newHeight);
  void paintGL();

  unsigned int maxInt;

  void renderMesh();

  void mousePressEvent(QMouseEvent* event);
  void wheelEvent(QWheelEvent* event);
  void keyPressEvent(QKeyEvent* event);

private:
  QOpenGLDebugLogger* debugLogger;

  QMatrix4x4 modelViewMatrix, projectionMatrix;
  QMatrix3x3 normalMatrix;

  // Uniforms
  GLint uniModelViewMatrix, uniProjectionMatrix, uniNormalMatrix;
  GLint tessUniModelViewMatrix, tessUniProjectionMatrix, tessUniNormalMatrix;


  // ---

  QOpenGLShaderProgram* mainShaderProg, *tesselationShaderProg;

  GLuint meshVAO, meshCoordsBO, meshNormalsBO, meshIndexBO;
  GLuint quadVAO, quadCoordsBO, quadIndexBO;
  unsigned int meshIBOSize;

  // ---

  QVector<QVector3D> vertexCoords;
  QVector<QVector3D> vertexNormals;
  QVector<unsigned int> polyIndices;

  QVector<QVector3D> quadCoords;
  QVector<unsigned int> quadIndices;


  void createShaderPrograms();
  void createBuffers();  

private slots:
  void onMessageLogged( QOpenGLDebugMessage Message );

};

#endif // MAINVIEW_H
