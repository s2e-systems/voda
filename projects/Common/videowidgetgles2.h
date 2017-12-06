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

#ifndef VIDEOWIDGETES2_H
#define VIDEOWIDGETES2_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QSize>
#include <QRect>
#include <QImage>

// Forward declarations
class QOpenGLDebugLogger;
class QOpenGLShaderProgram;

/**
 * Extents the QOpenGLWidget by one texture that is rendered
 * on the widget surface preserving the aspect ratio set by
 * setAspectRatio().
 * This class uses the QOpenGLFunctions functions, that means
 * it is OpenGL 2.0 and ES compatible and can be used e.g. on a
 * raspberry pi.
 * To draw to the texture glTexImage2D(GL_TEXTURE_2D, ...) function
 * can be used.
 * A convenients function to draw a QImage to the texture is
 * provided by drawImage().
 * The paintGL() function paints the texture. For this function to be called
 * update() may be called. The update function get also called if the widget
 * is for example resized.
 */
class VideoWidgetGLES2 : public QOpenGLWidget, protected QOpenGLFunctions
{
public:

	/**
	 * Sets default aspect ratio and set the contex of OpenGL to be debugging mode.
	 */
	VideoWidgetGLES2(QWidget* parent = nullptr, Qt::WindowFlags windowFlags = Qt::WindowFlags());

	/**
	 * Returns the aspect ratio.
	 * This is an overwritten function that is used by the qt widget system.
	 */
	virtual QSize sizeHint() const;

	/**
	 * Transferres the image data to the OpenGL using glTexImage2D(GL_TEXTURE_2D, ...)
	 * A limit set of format is supported. A warning is given if a format is not supported.
	 */
	virtual void drawImage(QImage const& image);

	/**
	 * Bins the texture m_textureName to the 2D texure buffer using
	 * glBindTexture(GL_TEXTURE_2D, m_textureName).
	 *
	 * This is for possible derived classes that can overwrite this function such
	 * that a different buffer may be used without having to change all occurances.
	 *
	 * This can (and probably is) called from paintGL to enable the wanted
	 * texture buffer at every painting.
	 */
	virtual void bindTexture();

	/**
	 * Prints all meesages from the GL message logger and removes them from the pipe.
	 * This uses QDebug()
	 */
	void openGLdebug(QString const& msg = QString());

	/**
	 * Gets the current aspect ratio.
	 */
	QSize aspectRatio() const;

	/**
	 * Sets the aspect ratio of what will be drawn.
	 * This is then used by the paintGL function to determine the maximum rectangle
	 * to draw on the current widget size.
	 */
	void setAspectRatio(const QSize& aspectRatio);

	/**
	 * Returns the current viewport rectangle. This is comuted ar a resize event from
	 * the current widget size and the aspect ratio.
	 */
	QRect vpRect() const;

protected:

	/**
	 * Constructs the vertex and fragment shaders (the actual source code of the shaders) as
	 * a string. Initializes OpenGL function and the shaders.
	 * Creates and transferes vertex data and texture data to the GPU.
	 * Prepares a 2D texture buffer under the name m_textureName.
	 */
	void initializeGL();

	/**
	 * Overwritten from #OpenGlWidget
	 * Bind the texture buffer with bindTexture(). Clears the color and depth buffer
	 * resets the viewport to preserve the aspect ratio.
	 */
	void paintGL();

	/**
	 * Overwritten from #OpenGlWidget
	 * Computes the viewport rectangle to match the current widget surface size.
	 */
	void resizeGL(int width, int height);

	/**
	 * Switch between fullscreen and normal.
	 */
	virtual void mouseDoubleClickEvent(QMouseEvent* event);

private:
	QOpenGLDebugLogger* m_glLogger;

	QOpenGLShaderProgram* m_shader;
	GLuint m_textureName;

	QSize m_aspectRatio;
	QRect m_vpRect;

	QOpenGLBuffer m_arrayBuf;
};

#endif
