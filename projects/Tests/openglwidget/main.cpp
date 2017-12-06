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

#include <QDebug>

#include <QApplication>
#include <QSurfaceFormat>
#include <QImage>

#include "testvideowidget.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setOption(QSurfaceFormat::DebugContext);
	QSurfaceFormat::setDefaultFormat(format);

	VideoWidgetGLES2 widget;

	widget.show();

	QImage img(300, 200, QImage::Format_RGBA8888_Premultiplied);
	img.fill(Qt::green);
	widget.drawImage(img);
	widget.update();

	return app.exec();
}
