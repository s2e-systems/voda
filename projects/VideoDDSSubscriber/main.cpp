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

#include "videoddssubscriber.h"
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
	VideoDDSsubscriber application(argc, argv);

	QCommandLineParser parser;
	parser.setApplicationDescription("Test VideoDDSsubscriber");
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption useOmxOption("omx", "Use omx as the decoder");
	parser.addOption(useOmxOption);

	parser.process(application);

	application.setUseOmx(parser.isSet(useOmxOption));

	qDebug() << "This is" << application.applicationName();

	application.init();
	return application.exec();
}
