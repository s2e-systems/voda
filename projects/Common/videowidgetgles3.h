// Copyright 2017 S2E Software, Systems and Control 
//  
// Licensed under the Apache License, Version 2.0 the "License"; 
// you may not use this file except in compliance with the License. 
// You may obtain a copy of the License at 
//  
//    http://www.apache.org/licenses/LICENSE-2.0 
//  
// Unless required by applicable law or agreed to in writing, software 
// distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and 
// limitations under the License.

#ifndef VIDEOWIDGETES3_H
#define VIDEOWIDGETES3_H

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QSize>
#include <QRect>
#include <QImage>

class QOpenGLDebugLogger;
class QOpenGLShaderProgram;

//class QOpenGLExtension_OES_EGL_image;

class VideoWidgetGLES3 : public QOpenGLWidget, public QOpenGLExtraFunctions
{
public:
	VideoWidgetGLES3(QWidget* parent = nullptr, Qt::WindowFlags windowFlags = Qt::WindowFlags());
	virtual QSize sizeHint() const;
	virtual void drawImage(QImage const& image);
	virtual void bindTexture();

	void openGLdebug(QString const& msg = QString());

	QSize aspectRatio() const;
	void setAspectRatio(const QSize& aspectRatio);

	QRect vpRect() const;

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	virtual void mouseDoubleClickEvent(QMouseEvent* event);

private:
	QOpenGLDebugLogger* m_glLogger;

	QOpenGLShaderProgram* m_shader;
	GLuint m_vertexArrayObject;
	GLuint m_textureName;

	QSize m_aspectRatio;
	QRect m_vpRect;

//	QOpenGLExtension_OES_EGL_image* m_oes;
};

#endif // VIDEOWIDGETES_H
