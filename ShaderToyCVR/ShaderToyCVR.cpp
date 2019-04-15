#include "ShaderToyCVR.hpp"

#include <cvrMenu/MenuSystem.h>
#include <cvrKernel/PluginHelper.h>

#include <osg/Depth>
#include <osgUtil/CullVisitor>

#include <ctime>
#include <fstream>
#include <string>
#include <sstream>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

using namespace osg;
using namespace std;
using namespace cvr;

CVRPLUGIN(ShaderToyCVR)

ShaderToyCVR::ShaderToyCVR() {}
ShaderToyCVR::~ShaderToyCVR() {}

static const string vertSource =
	"#version 400\n"
	"uniform mat4 osg_ModelViewProjectionMatrix;\n"
	"layout(location = 0) in vec4 osg_Vertex;\n"
	"out vec4 _clipPos;\n"
	"void main() {\n"
	"	gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;\n"
	"	_clipPos = gl_Position;\n"
	"}";

static const string fragSource =
	"#version 400\n"
	"uniform vec3 iResolution;"
	"uniform float iTime;"
	"uniform float iTimeDelta;"
	"uniform int iFrame;"
	"uniform float iChannelTime[4];"
	"uniform vec3 iChannelResolution[4];"
	"uniform vec4 iMouse;"
	"uniform sampler2D iChannel0;"
	"uniform sampler2D iChannel1;"
	"uniform sampler2D iChannel2;"
	"uniform sampler2D iChannel3;"
	"uniform vec4 iDate;"
	"uniform float iSampleRate;"
	"uniform mat4 iInverseViewMatrix;"
	"uniform mat4 iInverseProjectionMatrix;"
	"in vec4 _clipPos;\n";

static const string fragMain = 
	"void main() {\n"
	"	vec4 viewp = iInverseProjectionMatrix * vec4(_clipPos.xy, 1.0, 1.0);\n"
	"	viewp /= viewp.w;\n"
	"	vec4 wp = iInverseViewMatrix * viewp;\n"
	"	vec3 ro = iInverseViewMatrix[3].xyz / iInverseViewMatrix[3].w;\n"

	"	vec4 c;\n"
	"	//mainImage(c, gl_FragCoord.xy);\n"
	"	mainVR(c, gl_FragCoord.xy, ro, normalize(wp.xyz - ro));\n"
	"	gl_FragColor = c;\n"
	"	gl_FragColor.a = 1.0;\n"
	"}";

struct InverseViewMatrixCallback : public Uniform::Callback {
	osg::Camera* _camera;
	InverseViewMatrixCallback(Camera* camera) : _camera(camera) {}
	virtual void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv) {
		uniform->set(Matrix::inverse(computeLocalToWorld(nv->getNodePath()) * _camera->getViewMatrix()));
	}
};
struct InverseProjectionMatrixCallback : public Uniform::Callback {
	osg::Camera* _camera;
	InverseProjectionMatrixCallback(Camera* camera) : _camera(camera) {}
	virtual void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv) {
		uniform->set(Matrix::inverse(_camera->getProjectionMatrix()));
	}
};

void ShaderToyCVR::LoadShader(const string& name) {
	ifstream t("E:\\Data\\shadertoy\\" + name);
	printf("Loading shader %s\n", ("E:\\Data\\shadertoy\\" + name).c_str());

	string src = fragSource;

	stringstream ss;
	ss << t.rdbuf();
	src += ss.str();
	src += fragMain;
	
	mShader->removeShader(mActiveShader);
	mShader->addShader(mActiveShader = new Shader(Shader::FRAGMENT, src));
}

void ShaderToyCVR::CreateCube() {
	vertices = new Vec3Array();
	indices = new DrawElementsUInt(PrimitiveSet::TRIANGLES, 0);

	vertices->setName("ShaderToy Cube Vertices");
	vertices->push_back(Vec3f(-100000.f, -100000.f,  100000.f));
	vertices->push_back(Vec3f( 100000.f, -100000.f,  100000.f));
	vertices->push_back(Vec3f( 100000.f,  100000.f,  100000.f));
	vertices->push_back(Vec3f(-100000.f,  100000.f,  100000.f));
	vertices->push_back(Vec3f(-100000.f, -100000.f, -100000.f));
	vertices->push_back(Vec3f( 100000.f, -100000.f, -100000.f));
	vertices->push_back(Vec3f( 100000.f,  100000.f, -100000.f));
	vertices->push_back(Vec3f(-100000.f,  100000.f, -100000.f));

	indices->setName("ShaderToy Cube Indices");
	indices->push_back(0); indices->push_back(1); indices->push_back(2);
	indices->push_back(2); indices->push_back(3); indices->push_back(0);
	indices->push_back(1); indices->push_back(5); indices->push_back(6);
	indices->push_back(6); indices->push_back(2); indices->push_back(1);
	indices->push_back(7); indices->push_back(6); indices->push_back(5);
	indices->push_back(5); indices->push_back(4); indices->push_back(7);
	indices->push_back(4); indices->push_back(0); indices->push_back(3);
	indices->push_back(3); indices->push_back(7); indices->push_back(4);
	indices->push_back(4); indices->push_back(5); indices->push_back(1);
	indices->push_back(1); indices->push_back(0); indices->push_back(4);
	indices->push_back(3); indices->push_back(2); indices->push_back(6);
	indices->push_back(6); indices->push_back(7); indices->push_back(3);

	Geometry* geometry = new Geometry();
	geometry->setVertexArray(vertices);
	geometry->addPrimitiveSet(indices);

	Geode* geode = new Geode();
	geode->addDrawable(geometry);

	mShader = new Program();
	mShader->setName("ShaderToy");
	mShader->addShader(new Shader(Shader::VERTEX, vertSource));
	mShader->addShader(mActiveShader = new Shader(Shader::FRAGMENT, fragSource + "void main(){gl_FragColor = vec4(0,1,0,1);}"));

	iInverseViewMatrix = new Uniform(Uniform::FLOAT_MAT4, "iInverseViewMatrix");
	iInverseProjectionMatrix = new Uniform(Uniform::FLOAT_MAT4, "iInverseProjectionMatrix");

	iResolution = new Uniform(Uniform::FLOAT_VEC3, "iResolution");
	iTime = new Uniform(Uniform::FLOAT, "iTime");
	iTimeDelta = new Uniform(Uniform::FLOAT, "iTimeDelta");
	iFrame = new Uniform(Uniform::INT, "iFrame");
	iChannelTime = new Uniform(Uniform::FLOAT, "iChannelTime", 4);
	iChannelResolution = new Uniform(Uniform::FLOAT_VEC3, "iChannelResolution", 4);
	iMouse = new Uniform(Uniform::FLOAT_VEC4, "iChannelResolution");
	iChannel0 = new Uniform(Uniform::SAMPLER_2D, "iChannel0");
	iChannel1 = new Uniform(Uniform::SAMPLER_2D, "iChannel1");
	iChannel2 = new Uniform(Uniform::SAMPLER_2D, "iChannel2");
	iChannel3 = new Uniform(Uniform::SAMPLER_2D, "iChannel3");
	iDate = new Uniform(Uniform::FLOAT_VEC4, "iDate");
	iSampleRate = new Uniform(Uniform::FLOAT, "iSampleRate");

	mState = geode->getOrCreateStateSet();
	mState->setAttributeAndModes(new Depth(Depth::LEQUAL, 1.f, 1.f));
	mState->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	mState->setAttributeAndModes(mShader, StateAttribute::ON);

	mState->addUniform(iInverseViewMatrix);
	mState->addUniform(iInverseProjectionMatrix);

	mState->addUniform(iResolution);
	mState->addUniform(iTime);
	mState->addUniform(iTimeDelta);
	mState->addUniform(iFrame);
	mState->addUniform(iChannelTime);
	mState->addUniform(iChannelResolution);
	mState->addUniform(iMouse);
	mState->addUniform(iChannel0);
	mState->addUniform(iChannel1);
	mState->addUniform(iChannel2);
	mState->addUniform(iChannel3);
	mState->addUniform(iDate);
	mState->addUniform(iSampleRate);

	mSceneObject = new SceneObject("ShaderToy", true, false, false, true, false);
	mSceneObject->addChild(geode);
	PluginHelper::registerSceneObject(mSceneObject, "ShaderToy");
	mSceneObject->attachToScene();
}

bool ShaderToyCVR::init() {
	mSubMenu = new SubMenu("ShaderToy", "ShaderToy");
	mSubMenu->setCallback(this);

	#ifdef WIN32

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile("E:\\Data\\shadertoy\\*.frag", &ffd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (ffd.cFileName[0] == '.') continue;

			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				printf("Loaded shader %s\n", ffd.cFileName);
				auto btn = new MenuButton(ffd.cFileName);
				btn->setCallback(this);
				mSubMenu->addItem(btn);
				mShaderButtons.push_back(btn);
			}
		} while (FindNextFile(hFind, &ffd) != 0);

		FindClose(hFind);
	} else
		printf("Failed to open shader directory\n");

	#endif

	MenuSystem::instance()->addMenuItem(mSubMenu);

	CreateCube();

	osgViewer::ViewerBase::Cameras cameras;
	CVRViewer::instance()->getCameras(cameras);
	for (const auto& c : cameras) {
		c->getGraphicsContext()->getState()->setUseModelViewAndProjectionUniforms(true);
		iInverseViewMatrix->setUpdateCallback(new InverseViewMatrixCallback(c));
		iInverseProjectionMatrix->setUpdateCallback(new InverseProjectionMatrixCallback(c));
	}

	return true;
}

void ShaderToyCVR::menuCallback(MenuItem* menuItem) {
	for (unsigned int i = 0; i < mShaderButtons.size(); i++) {
		if (menuItem == mShaderButtons[i]) {
			mStartTime = osg::Timer::instance()->tick();
			mFrame = 0;
			LoadShader(mShaderButtons[i]->getText());
			break;
		}
	}
}

void ShaderToyCVR::preFrame() {
	mFrame++;

	int w = 800, h = 600;

	osgViewer::ViewerBase::Contexts contexts;
	CVRViewer::instance()->getContexts(contexts);
	for (const auto& c : contexts) {
		w = c->getTraits()->width;
		h = c->getTraits()->height;
	}

	time_t nowt = time(0);
	tm* now = localtime(&nowt);

	Timer_t frameTick = osg::Timer::instance()->tick();

	iResolution->set(Vec3(w, h, 1));
	iTime->set((float)Timer::instance()->delta_s(mStartTime, frameTick));
	iTimeDelta->set((float)Timer::instance()->delta_s(mLastFrame, frameTick));
	iFrame->set(mFrame);
	iDate->set(Vec4(now->tm_year, now->tm_mon, now->tm_mday, now->tm_sec));
	iSampleRate->set(44100.0f);

	mLastFrame = frameTick;
}