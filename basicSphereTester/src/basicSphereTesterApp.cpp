#include "cinder/app/AppNative.h"
#include "cinder/Camera.h"
#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Arcball.h"
#include "cinder/Surface.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"
#include "cinder/ObjLoader.h"

#include "cinder/Sphere.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class basicSphereTesterApp : public AppNative {
  public:
	void setup();
	void update();
	void draw();
    
    //built in functions
    void prepareSettings(Settings *settings);
    void resize();
    void keyDown( KeyEvent event );
    void mouseDown(MouseEvent event);
    void mouseDrag(MouseEvent event);
    
    //my functions
    bool compileShaders();
    bool createTextures();
    bool createSphere();
    
    void renderDisplacementMap();
    void renderNormalMap();
    
    
    //camera
    MayaCamUI		mMayaCam;
    Arcball         mArcball;
	CameraPersp		mCamera;
    Vec3f           sceneCenter;
    
    //shaders and associated FBO's
    gl::Fbo			mDispMapFbo;
	gl::GlslProg	mDispMapShader;
    
	gl::Fbo			mNormalMapFbo;
	gl::GlslProg	mNormalMapShader;
	
    TriMesh         mMesh;
	gl::VboMesh		mVboMesh;
	gl::GlslProg	mMeshShader;
    
    //textures
    gl::Texture     surfaceTex;
    
    //control parameters
    float			mAmplitude;
    float           mRate;
    
    bool mEnableShader; //"fallof"?
    bool mDrawTextures;
    bool mDrawWireframe;
};

void basicSphereTesterApp::setup()
{
    sceneCenter = Vec3f(0, 0, 0);
    mCamera.setEyePoint( Vec3f( 0.0f, 0.0f, 130.0f ) );
	mCamera.setCenterOfInterestPoint( sceneCenter ) ;
	mMayaCam.setCurrentCam( mCamera );
    
    if (!compileShaders()) {quit();}
    if (!createTextures()) {quit();}
    if (!createSphere()) {quit();}
    
    
    // create the frame buffer objects for the displacement map and the normal map
	gl::Fbo::Format fmt; //used for both
	fmt.enableDepthBuffer(false);
	fmt.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
    
	// use a single channel (red) for the displacement map
	fmt.setColorInternalFormat( GL_R32F );
	mDispMapFbo = gl::Fbo(256, 256, fmt);
	
	// use 3 channels (rgb) for the normal map
	fmt.setColorInternalFormat( GL_RGB );
	mNormalMapFbo = gl::Fbo(256, 256, fmt);
    
    mAmplitude = .2;
    mRate = 1.0;
    
    mEnableShader = true;
    mDrawTextures = false;
    mDrawWireframe = false;
}

void basicSphereTesterApp::update()
{
    // render displacement map
	renderDisplacementMap();
    
	// render normal map
	renderNormalMap();
}

void basicSphereTesterApp::draw()
{
	// clear out the window with black
	gl::clear(Color(0.5, 0.0, 0.5));
    
    if( surfaceTex )
	{
        //surfaceTex.enableAndBind();
        //gl::color(0.2, 0.0, .8);
		//gl::draw( surfaceTex, getWindowBounds() );
        //surfaceTex.unbind();
	}
    
    // if enabled, show the displacement and normal maps
	if(mDrawTextures)
	{
		gl::color( Color(0.5f, 0.5f, 0.5f) );
		gl::draw( mDispMapFbo.getTexture(), Vec2f(0,0) );
		gl::color( Color(1.0f, 1.0f, 1.0f) );
		gl::draw( mNormalMapFbo.getTexture(), Vec2f(256,0) );
	}
    
    if ( mDispMapFbo && mNormalMapFbo && mMeshShader )
	{
        mDispMapFbo.getTexture().bind(0);
        mNormalMapFbo.getTexture().bind(1);
        surfaceTex.bind(2);
    
        // render our mesh using vertex displacement
        mMeshShader.bind();
        mMeshShader.uniform( "displacement_map", 0 );
        mMeshShader.uniform( "normal_map", 1 );
        mMeshShader.uniform( "surface_texture", 2 );
        mMeshShader.uniform( "falloff_enabled", mEnableShader );
        
        gl::enableAlphaBlending();
        if (mDrawWireframe) gl::enableWireframe();
        gl::enableDepthRead();
    
        gl::pushMatrices();
        gl::setMatrices(mCamera);
        gl::scale(Vec3f(25, 25, 25));
        gl::rotate( mArcball.getQuat() );
        gl::draw(mVboMesh);
        gl::popMatrices();
        
        gl::disableDepthRead();
        gl::disableAlphaBlending();
        gl::disableWireframe();
    
        mMeshShader.unbind();
        
        mNormalMapFbo.unbindTexture();
        mDispMapFbo.unbindTexture();
        
    }
    else quit();
}

void basicSphereTesterApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize( 1280, 720 );
    settings->setFrameRate( 500.0f ); //woah
    //other options
}

void basicSphereTesterApp::resize()
{
	// if window is resized, update camera aspect ratio
	mCamera.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( mCamera );
    
    mArcball.setWindowSize( getWindowSize() );
	mArcball.setCenter( Vec2f( getWindowWidth() / 2.0f, getWindowHeight() / 2.0f ) );
	mArcball.setRadius( 150 );
}

void basicSphereTesterApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
        case KeyEvent::KEY_ESCAPE:
		// quit
		quit();
		break;
        
        case KeyEvent::KEY_s:
		// reload shaders
		compileShaders();
		break;
        
        case KeyEvent::KEY_t:
		// toggle draw textures
		mDrawTextures = !mDrawTextures;
		break;
        
        case KeyEvent::KEY_q:
		mEnableShader = !mEnableShader;
		break;
        
        case KeyEvent::KEY_w:
        mDrawWireframe = !mDrawWireframe;
        break;
	}
}

void basicSphereTesterApp::mouseDown( MouseEvent event )
{
	if( event.isAltDown() )
    mMayaCam.mouseDown( event.getPos() );
	else
    mArcball.mouseDown( event.getPos() );
}

void basicSphereTesterApp::mouseDrag( MouseEvent event )
{
	if( event.isAltDown() )
    mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	else
    mArcball.mouseDrag( event.getPos() );
}

bool basicSphereTesterApp::compileShaders()
{
	try
	{
		// this shader will render a displacement map to a floating point texture, updated every frame
		mDispMapShader = gl::GlslProg( loadResource("displacement_map_vert.glsl"), loadResource("displacement_map_perlin_frag.glsl"));
		// this shader will create a normal map based on the displacement map
		mNormalMapShader = gl::GlslProg( loadResource("normal_map_vert.glsl"), loadResource("normal_map_frag.glsl") );
		// this shader will use the displacement and normal maps to displace vertices of a mesh
		mMeshShader = gl::GlslProg( loadResource("mesh_vert.glsl"), loadResource("mesh_frag.glsl") );
	}
	catch( const std::exception &e )
	{
		console() << e.what() << std::endl;
		return false;
	}
    
	return true;
}

void basicSphereTesterApp::renderDisplacementMap()
{
	if( mDispMapShader && mDispMapFbo )
	{
		mDispMapFbo.bindFramebuffer();
		{
			// clear the color buffer
			gl::clear();
            
			// setup viewport and matrices
			glPushAttrib( GL_VIEWPORT_BIT );
			gl::setViewport( mDispMapFbo.getBounds() );
            
			gl::pushMatrices();
			gl::setMatricesWindow( mDispMapFbo.getSize(), false ); //false makes the lower-right the origin
            
			// render the displacement map
			mDispMapShader.bind();
			mDispMapShader.uniform( "time", (float) getElapsedSeconds() * mRate);
			mDispMapShader.uniform( "amplitude", mAmplitude );
            mDispMapShader.uniform( "octaves", 8 );
            mDispMapShader.uniform( "lacunarity", (float) 4.0 );
			gl::drawSolidRect( mDispMapFbo.getBounds() );
			mDispMapShader.unbind();
            
			// clean up after ourselves
			gl::popMatrices();
			glPopAttrib();
		}
		mDispMapFbo.unbindFramebuffer();
    }
}

void basicSphereTesterApp::renderNormalMap()
{
	if( mNormalMapShader && mNormalMapFbo)
	{
		mNormalMapFbo.bindFramebuffer();
		{
			// setup viewport and matrices
			glPushAttrib( GL_VIEWPORT_BIT );
			gl::setViewport( mNormalMapFbo.getBounds() );
            
			gl::pushMatrices();
			gl::setMatricesWindow( mNormalMapFbo.getSize(), false ); //false makes the lower-right the origin
            
			// clear the color buffer
			gl::clear();
            
			// bind the displacement map
			mDispMapFbo.getTexture().bind(0);
            
			// render the normal map
			mNormalMapShader.bind();
			mNormalMapShader.uniform( "texture", 0 );
			mNormalMapShader.uniform( "amplitude", 1/mAmplitude ); //was 4.0 in the thing
            
			Area bounds = mNormalMapFbo.getBounds(); //bounds.expand(-1, -1);
			gl::drawSolidRect( bounds );
            
			mNormalMapShader.unbind();
            
			// clean up after ourselves
			mDispMapFbo.getTexture().unbind();
            
			gl::popMatrices();
            
			glPopAttrib();
		}
		mNormalMapFbo.unbindFramebuffer();
	}
}

bool basicSphereTesterApp::createTextures()
{
	try {
        surfaceTex = gl::Texture( loadImage( loadResource( "Blackened-Chicken-Pre-Cooked.jpg") ) );
        surfaceTex.setWrap(GL_REPEAT, GL_REPEAT);
	}
	catch( const std::exception &e ) {
		console() << "Could not load image: " << e.what() << std::endl;
        return false;
	}
    return true;
}

bool basicSphereTesterApp::createSphere()
{
    try {
        ObjLoader loader( (DataSourceRef) loadResource("sphere.obj") );
        //kinda silly for exception handling. in a real sitch, Idwanna initialize ObjLoader, and only load the object in the block
        loader.load(&mMesh);
        mVboMesh = gl::VboMesh(mMesh);
    }
    catch(...)
    {
        cout << "failed to load object! goodbye!\n";
        return false;
    }
    return true;
}



CINDER_APP_NATIVE( basicSphereTesterApp, RendererGl )
