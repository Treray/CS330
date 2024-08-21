///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
	glfwSetScrollCallback(window, &ViewManager::scroll_callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	////Reference: LearnOpenGL - Camera. (n.d.).https://learnopengl.com/Getting-started/Camera  TR
	if (gFirstMouse)
	{
		gLastX = xMousePos;// Stores the mouse initial x position 
		gLastY = yMousePos;//Stores the mouse initial y position
		gFirstMouse = false;
	}

	float xoffset = xMousePos - gLastX;
	float yoffset = gLastY - yMousePos;
	gLastX = xMousePos;
	gLastY = yMousePos;


	//Sensitivity is defined for the mouse movement 
	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;


	// Update the camera's yaw and pitch
	g_pCamera->Yaw += xoffset;
	g_pCamera->Pitch += yoffset;

	// Pitch value is clamped to prevent camera from flipping
	if (g_pCamera->Pitch > 89.0f)
		g_pCamera->Pitch = 89.0f;
	if (g_pCamera->Pitch < -89.0f)
		g_pCamera->Pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
	direction.y = sin(glm::radians(g_pCamera->Pitch));
	direction.z = sin(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
	g_pCamera->Front = glm::normalize(direction);
}


/***********************************************************
 *  scroll_callback()
 *
 *  This method is called from GLFW when the mouse wheel is scrolled.
 ***********************************************************/

 //Reference: LearnOpenGL - Camera. (n.d.).https://learnopengl.com/Getting-started/Camera  TR
void ViewManager::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{


	//  movement speed is clamped 
	g_pCamera->MovementSpeed += (float)yoffset;
	if (g_pCamera->MovementSpeed < 1.0f)
		g_pCamera->MovementSpeed = 1.0f;
	if (g_pCamera->MovementSpeed > 45.0f)
		g_pCamera->MovementSpeed = 45.0f;



}



/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	
	// Close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// If the camera object is null, then exit this method
	if (NULL == g_pCamera)
	{
		return;
	}

	// Process camera zooming in and out
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// Process camera panning left and right
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	// Camera moves up and down using Q and E
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}
	// change between different projection views
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		// change to a multi-view orthographic projection
		bOrthographicProjection = true;

		// change the camera settings to show a front orthographic view
		g_pCamera->Position = glm::vec3(0.0f, 4.0f, 10.0f);
		g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
		g_pCamera->Front = glm::vec3(0.0f, 0.0f, -1.0f);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_2) == GLFW_PRESS)
	{
		// change to a multi-view orthographic projection
		bOrthographicProjection = true;

		// change the camera settings to show a side orthographic view
		g_pCamera->Position = glm::vec3(10.0f, 4.0f, 0.0f);
		g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
		g_pCamera->Front = glm::vec3(-1.0f, 0.0f, 0.0f);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_3) == GLFW_PRESS)
	{
		// change to a multi-view orthographic projection
		bOrthographicProjection = true;

		// change the camera settings to show a top orthographic view
		g_pCamera->Position = glm::vec3(0.0f, 7.0f, 0.0f);
		g_pCamera->Up = glm::vec3(-1.0f, 0.0f, 0.0f);
		g_pCamera->Front = glm::vec3(0.0f, -1.0f, 0.0f);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		// change to perspective projection
		bOrthographicProjection = false;

		// change the camera settings to show a perspective view
		g_pCamera->Position = glm::vec3(0.0f, 5.5f, 8.0f);
		g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
		g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
		g_pCamera->Zoom = 80;
	}
}
	
	

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// define the current projection matrix
	if (bOrthographicProjection == false)
	{
		// perspective projection
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else
	{
		// front-view orthographic projection with correct aspect ratio
		double scale = 0.0;
		if (WINDOW_WIDTH > WINDOW_HEIGHT)
		{
			scale = (double)WINDOW_HEIGHT / (double)WINDOW_WIDTH;
			projection = glm::ortho(-5.0f, 5.0f, -5.0f * (float)scale, 5.0f * (float)scale, 0.1f, 100.0f);
		}
		else if (WINDOW_WIDTH < WINDOW_HEIGHT)
		{
			scale = (double)WINDOW_WIDTH / (double)WINDOW_HEIGHT;
			projection = glm::ortho(-5.0f * (float)scale, 5.0f * (float)scale, -5.0f, 5.0f, 0.1f, 100.0f);
		}
		else
		{
			projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
		}
	}
	// define the current projection matrix
	projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}