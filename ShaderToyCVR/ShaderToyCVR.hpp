#pragma once

#include <cvrKernel/CVRPlugin.h>
#include <cvrKernel/SceneObject.h>
#include <cvrMenu/SubMenu.h>
#include <cvrMenu/MenuButton.h>

#include <osg/Program>
#include <osg/Uniform>
#include <osg/Timer>

class ShaderToyCVR : public cvr::MenuCallback, public cvr::CVRPlugin {
public:
	ShaderToyCVR();
	virtual ~ShaderToyCVR();

	void menuCallback(cvr::MenuItem* item) override;

	virtual bool init() override;
	virtual void preFrame() override;

private:
	void CreateCube();
	void LoadShader(const std::string& name);

	cvr::SubMenu* mSubMenu;
	std::vector<cvr::MenuButton*> mShaderButtons;

	osg::Vec3Array* vertices;
	osg::DrawElementsUInt* indices;

	osg::Shader* mActiveShader;
	osg::Program* mShader;
	osg::StateSet* mState;
	cvr::SceneObject* mSceneObject;

	osg::Uniform* iModelMatrix;
	osg::Uniform* iViewMatrix;
	osg::Uniform* iProjectionMatrix;

	osg::Uniform* iResolution;
	osg::Uniform* iTime;
	osg::Uniform* iTimeDelta;
	osg::Uniform* iFrame;
	osg::Uniform* iChannelTime;
	osg::Uniform* iChannelResolution;
	osg::Uniform* iMouse;
	osg::Uniform* iChannel0;
	osg::Uniform* iChannel1;
	osg::Uniform* iChannel2;
	osg::Uniform* iChannel3;
	osg::Uniform* iDate;
	osg::Uniform* iSampleRate;

	int mFrame;
	osg::Timer_t mStartTime;
	osg::Timer_t mLastFrame;
};