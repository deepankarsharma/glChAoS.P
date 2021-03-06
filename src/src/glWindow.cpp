////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2018 Michele Morrone
//  All rights reserved.
//
//  mailto:me@michelemorrone.eu
//  mailto:brutpitt@gmail.com
//  
//  https://github.com/BrutPitt
//
//  https://michelemorrone.eu
//  https://BrutPitt.com
//
//  This software is distributed under the terms of the BSD 2-Clause license:
//  
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright
//        notice, this list of conditions and the following disclaimer.
//      * Redistributions in binary form must reproduce the above copyright
//        notice, this list of conditions and the following disclaimer in the
//        documentation and/or other materials provided with the distribution.
//   
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
////////////////////////////////////////////////////////////////////////////////
#include <glm/glm.hpp>


#include "glWindow.h"
#include "ParticlesUtils.h"

//Random numbers of particle velocity of fragmentation
RandomTexture rndTexture;
HLSTexture hlsTexture;


// 
////////////////////////////////////////////////////////////////////////////
void glWindow::onInit()
{

    glViewport(0,0,theApp->GetWidth(), theApp->GetHeight());
    vao = new vaoClass;
    
    //rndTexture.buildTex(1024);
    //hlsTexture.buildTex(1024);

    //modelMatrix = projectionMatrix = viewMatrix = mvpMatrix = mvMatrix = glm::mat4(1.0f);

    //particlesSystem = new particlesSystemClass(new transformedEmitterClass(1,mVB_COLOR));
    particlesSystem = new particlesSystemClass(new singleEmitterClass);
    //shaderMotionBlur.create();

#if !defined(__EMSCRIPTEN__)
    glEnable( GL_PROGRAM_POINT_SIZE );
    glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
    GLfloat retVal[4];
    glGetFloatv(GL_POINT_SIZE_RANGE, &retVal[0]);
    particlesSystem->shaderPointClass::getUData().pointspriteMinSize = retVal[0];
#endif

    attractorsList.newStepThread(particlesSystem->getEmitter());
    attractorsList.setSelection(0);
    attractorsList.getThreadStep()->startThread();

    //if(loadAttractorFile(false, ATTRACTOR_PATH "mainMagnetic" ATTRACTOR_EXT)) return;



    particlesSystem->getTMat()->setPerspective(30.f, float(theApp->GetWidth())/float(theApp->GetHeight()), 0.f, 100.f);
    particlesSystem->getTMat()->setView(attractorsList.get()->getPOV(), attractorsList.get()->getTGT());

    particlesSystem->getTMat()->getTrackball().setGizmoRotControl( (vgButtons) GLFW_MOUSE_BUTTON_LEFT, (vgModifiers) 0 /* evNoModifier */ );

    particlesSystem->getTMat()->getTrackball().setGizmoRotXControl((vgButtons) GLFW_MOUSE_BUTTON_LEFT, (vgModifiers) GLFW_MOD_SHIFT);
    particlesSystem->getTMat()->getTrackball().setGizmoRotYControl((vgButtons) GLFW_MOUSE_BUTTON_LEFT, (vgModifiers) GLFW_MOD_CONTROL);
    particlesSystem->getTMat()->getTrackball().setGizmoRotZControl((vgButtons) GLFW_MOUSE_BUTTON_LEFT, (vgModifiers) GLFW_MOD_ALT | GLFW_MOD_SUPER);

    particlesSystem->getTMat()->getTrackball().setDollyControl((vgButtons) GLFW_MOUSE_BUTTON_RIGHT, (vgModifiers) 0);
    particlesSystem->getTMat()->getTrackball().setPanControl(  (vgButtons) GLFW_MOUSE_BUTTON_RIGHT, (vgModifiers) GLFW_MOD_CONTROL|GLFW_MOD_SHIFT);


    //particlesSystem->getTMat()->getTrackball().setDollyControl((vgButtons) GLFW_MOUSE_BUTTON_LEFT, (vgModifiers) GLFW_MOD_CONTROL);
    //particlesSystem->getTMat()->getTrackball().setPanControl((vgButtons) GLFW_MOUSE_BUTTON_LEFT, (vgModifiers)GLFW_MOD_SHIFT);

    //trackball.setDollyPosition(5.0f);
    particlesSystem->getTMat()->getTrackball().setRotationCenter(attractorsList.get()->getTGT());

    particlesSystem->getTMat()->getTrackball().viewportSize(theApp->GetWidth(), theApp->GetHeight());
    //applyRotation(modelMatrix, quaternionf(0.f,0.f,0.f,1.f));
    //applyRotation(modelMatrix, glm::quat(0.f,0.f,0.f,1.f));



    mmFBO::Init(theApp->GetWidth(), theApp->GetHeight()); 


}


// 
////////////////////////////////////////////////////////////////////////////
void glWindow::onExit()
{
    attractorsList.deleteStepThread();

    delete particlesSystem;
    delete vao;
}


// 
////////////////////////////////////////////////////////////////////////////
void glWindow::onRender()
{
    transformsClass *model = getParticlesSystem()->getTMat();

    //  render ColorMaps: rebuild texture only if settings are changed
    //////////////////////////////////////////////////////////////////
    particlesSystem->shaderPointClass::getCMSettings()->render();

#if !defined(GLCHAOSP_LIGHTVER)

    particlesSystem->shaderBillboardClass::getCMSettings()->render();

    transformsClass *axes = getParticlesSystem()->getAxes()->getTransforms();

    auto syncAxes = [&] () {
        axes->setView(model->getPOV(), getParticlesSystem()->getTMat()->getTGT());    
        axes->setPerspective(model->getPerspAngle(), float(theApp->GetWidth())/float(theApp->GetHeight()), model->getPerspNear(), model->getPerspFar() );
        axes->getTrackball().setRotation(model->getTrackball().getRotation());
    };

    if(particlesSystem->showAxes() == renderBaseClass::showAxesToSetCoR) {
    //  Set center of rotation
    //////////////////////////////////////////////////////////////////
        syncAxes();

        // no dolly & pan: axes are to center
        axes->getTrackball().setPanPosition(vec3(0.0));
        axes->getTrackball().setDollyPosition(vec3(0.0));

        // get rotation & translation of model w/o pan & dolly
        quat q =   model->getTrackball().getRotation() ;
        model->tM.mMatrix = glm::mat4_cast(q) * glm::translate(mat4(1.f), model->getTrackball().getRotationCenter());
        model->build_MV_MVP();

        // apply rotation to matrix... then subtract prevous model translation
        axes->tM.mMatrix = mat4(1.f); 
        axes->getTrackball().applyRotation(axes->tM.mMatrix); 
        axes->tM.mMatrix = translate(axes->tM.mMatrix , -model->getTrackball().getRotationCenter());
        axes->build_MV_MVP();


    } else  {
    //  Show center of rotation
    //////////////////////////////////////////////////////////////////
        if(particlesSystem->showAxes() == renderBaseClass::showAxesToViewCoR) {
            syncAxes();

            // add RotCent component to dolly & pan... to translate axes with transforms
            vec3 v(model->getTrackball().getRotationCenter());
            axes->getTrackball().setPanPosition(model->getTrackball().getPanPosition()-vec3(v.x, v.y, 0.0));
            axes->getTrackball().setDollyPosition(model->getTrackball().getDollyPosition()-vec3(0.0, 0.0, v.z));

            axes->applyTransforms();
        }

        model->applyTransforms();
    }
    
    //  render Attractor
    //////////////////////////////////////////////////////////////////
    GLuint texRendered = particlesSystem->render();

    //  Motion Blur
    //////////////////////////////////////////////////////////////////
    if(particlesSystem->getMotionBlur()->Active()) {

    //glDisable(GL_BLEND);
    #ifdef GLAPP_REQUIRE_OGL45
        glBlitNamedFramebuffer(particlesSystem->getMotionBlur()->render(texRendered),
                               0,
                               0,0,particlesSystem->getWidth(), particlesSystem->getHeight(),
                               0,0,theApp->GetWidth(), theApp->GetHeight(),
                               GL_COLOR_BUFFER_BIT, GL_NEAREST );
    #else
        glBindFramebuffer(GL_READ_FRAMEBUFFER, particlesSystem->getMotionBlur()->render(texRendered));
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(0,0,particlesSystem->getWidth(), particlesSystem->getHeight(),
                          0,0,theApp->GetWidth(), theApp->GetHeight(),
                          GL_COLOR_BUFFER_BIT, GL_NEAREST );
    #endif
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // bind both FRAMEBUFFERS to default
#else 

    model->applyTransforms();
    GLuint texRendered = particlesSystem->render();

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // bind both FRAMEBUFFERS to default
#endif

    //

    particlesSystem->clearFlagUpdate();

}



void glWindow::DrawOnTexture()
{

    
}



////////////////////////////////////////////////////////////////////////////
void glWindow::onIdle()
{
    particlesSystem->getTMat()->getTrackball().idle();
}


////////////////////////////////////////////////////////////////////////////
void glWindow::onReshape(GLint w, GLint h)
{
    glViewport(0,0,w,h);


    if(particlesSystem) particlesSystem->onReshape(w,h);

    theApp->SetWidth(w); theApp->SetHeight(h);
    particlesSystem->getTMat()->getTrackball().viewportSize(w, h);
}


////////////////////////////////////////////////////////////////////////////
void glWindow::onMouseButton(int button, int upOrDown, int x, int y)
{
    particlesSystem->getTMat()->getTrackball().mouse((vgButtons) (button),
                                                     (vgModifiers) theApp->getModifier(),
                                                      upOrDown==APP_MOUSE_BUTTON_DOWN, x, y );
}

////////////////////////////////////////////////////////////////////////////
void glWindow::onMotion(int x, int y)
{
    particlesSystem->getTMat()->getTrackball().motion(x, y);
}

////////////////////////////////////////////////////////////////////////////
void glWindow::onPassiveMotion(int x, int y) {}

////////////////////////////////////////////////////////////////////////////
void glWindow::onKeyUp(unsigned char key, int x, int y) {}

////////////////////////////////////////////////////////////////////////////
void glWindow::onSpecialKeyDown(int key, int x, int y) {}

////////////////////////////////////////////////////////////////////////////
void glWindow::onKeyDown(unsigned char key, int x, int y) {}

////////////////////////////////////////////////////////////////////////////
void glWindow::onSpecialKeyUp(int key, int x, int y) {}

////////////////////////////////////////////////////////////////////////////
void glWindow::onMouseWheel(int wheel, int direction, int x, int y) {}

