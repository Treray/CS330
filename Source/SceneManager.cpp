///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	//Reference:https://learn.snhu.edu/content/enforced/1644154-CS-330-11664.202456-1/course_documents/CS%20330%20Applying%20Lighting%20to%20a%203D%20Scene.pdf?isCourseFile=true&ou=1644154

	// Light Material
	OBJECT_MATERIAL lightMaterial;
	lightMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Low ambient color
	lightMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // Neutral diffuse color
	lightMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f); // Some specular highlights
	lightMaterial.shininess = 10.0f;
	lightMaterial.ambientStrength = 0.1f;
	lightMaterial.tag = "LightMaterial";
	m_objectMaterials.push_back(lightMaterial);

	// Monitor Material
	OBJECT_MATERIAL monitorMaterial;
	monitorMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Low ambient 
	monitorMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f); // Dark diffuse for material
	monitorMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // High specular for screen reflections
	monitorMaterial.shininess = 32.0f; 
	monitorMaterial.tag = "MonitorMaterial";
	m_objectMaterials.push_back(monitorMaterial);

	// Reflective Material for Floor
	OBJECT_MATERIAL reflectiveMaterial;
	reflectiveMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Low ambient
	reflectiveMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // Neutral diffuse 
	reflectiveMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f); // High specular reflections
	reflectiveMaterial.shininess = 64.0f; // High shininess for reflections
	reflectiveMaterial.ambientStrength = 0.1f;
	reflectiveMaterial.tag = "ReflectPlane";
	m_objectMaterials.push_back(reflectiveMaterial);

	// Desk Material
	OBJECT_MATERIAL deskMaterial;
	deskMaterial.ambientColor = glm::vec3(0.6f, 0.3f, 0.1f); // Brown ambient color for desk
	deskMaterial.diffuseColor = glm::vec3(0.6f, 0.3f, 0.1f); // Brown diffuse 
	deskMaterial.specularColor = glm::vec3(0.3f, 0.2f, 0.1f); // low specular for desk
	deskMaterial.shininess = 8.0f; 
	deskMaterial.tag = "DeskMaterial";
	m_objectMaterials.push_back(deskMaterial);

	// Monitor Stand Material
	OBJECT_MATERIAL standMaterial;
	standMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f); // Light gray for stand
	standMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f); // light gray diffuse 
	standMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f); // specular highlights metal stand
	standMaterial.shininess = 16.0f; 
	standMaterial.tag = "StandMaterial";
	m_objectMaterials.push_back(standMaterial);

	// PS5 Material
	OBJECT_MATERIAL ps5Material;
	ps5Material.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); // Medium dark ambient
	ps5Material.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f); // Light gray diffuse
	ps5Material.specularColor = glm::vec3(0.7f, 0.7f, 0.7f); // Moderate specular 
	ps5Material.shininess = 32.0f; // Balanced shininess 
	ps5Material.tag = "PS5Material";
	m_objectMaterials.push_back(ps5Material);


	// Speaker Material
	OBJECT_MATERIAL speakerMaterial;
	speakerMaterial.ambientColor = glm::vec3(0.9f, 0.9f, 0.9f); // Light ambient
	speakerMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f); // white diffuse
	speakerMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f); // High specular f
	speakerMaterial.shininess = 64.0f; // High shininess 
	speakerMaterial.tag = "SpeakerMaterial";
	m_objectMaterials.push_back(speakerMaterial);


}



/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/
	  // This line of code is NEEDED for telling the shaders to render
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	
	//Reference:https://learn.snhu.edu/content/enforced/1644154-CS-330-11664.202456-1/course_documents/CS%20330%20Applying%20Lighting%20to%20a%203D%20Scene.pdf?isCourseFile=true&ou=1644154
	// Light Source 1 Gold light covering scene 
	m_pShaderManager->setVec3Value("lightSources[1].position", 3.0f, 10.0f, -24.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.05f, 0.05f, 0.025f); 
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.5f, 0.4f, 0.2f); 
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.5f, 0.4f, 0.2f); 
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 8.0f); 
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 1.0f);

	// Light Source 2 white light from above
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 20.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.05f, 0.05f, 0.05f); 
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.6f, 0.6f, 0.6f); 
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.5f, 0.5f, 0.5f); 
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 6.0f); 
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.5f);


	// Light Source  Light on Monitor
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, 2.7f, 2.0f); 
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.1f, 0.1f, 0.1f); 
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.7f, 0.7f, 0.7f); 
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.9f, 0.9f, 0.9f); 
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 2.0f); 
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 1.0f);




	m_pShaderManager->setBoolValue("bUseLighting", true);

}

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/



	//Reference:https://learn.snhu.edu/content/enforced/1644154-CS-330-11664.202456-1/course_documents/CS%20330%20Applying%20Textures%20to%203D%20Shapes.pdf?isCourseFile=true&ou=1644154

	//textures uploaded in to memory 
	bool bReturn = false;
	bReturn = CreateGLTexture(
		"../../Utilities/textures/floor.jpg",
		"floor");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/knife_handle.jpg",
		"wood");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/stainless.jpg",
		"stainless");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/stainless_end.jpg",
		"stainlessend");


	bReturn = CreateGLTexture(
		"../../Utilities/textures/Galaga.jpg",
		"game");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/Blackgloss.jpg",
		"Blackgloss");


	bReturn = CreateGLTexture(
		"../../Utilities/textures/Whitetex.jpg",
		"Whitetex");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/WhiteMarble.jpg",
		"Whitemarb");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
	// load the textures for the 3D scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();

	//added box and cylinder meshes to create the monitor.
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;



	/*** PS5 Shapes ************************************************/
	/******************************************************************/

	// PS5 Body
	SetShaderMaterial("PS5Material");//Shader material set for ps5
	scaleXYZ = glm::vec3(0.3f, 1.5f, 0.6f); 
	positionXYZ = glm::vec3(-2.0f, 2.25f, 0.25f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("Blackgloss");
	m_basicMeshes->DrawBoxMesh(); 

	// PS5 left Panel
	SetShaderMaterial("PS5Material");
	scaleXYZ = glm::vec3(0.03f, 1.7f, 0.7f); 
	positionXYZ = glm::vec3(-2.15f, 2.26f, 0.25f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("Whitetex"); 
	m_basicMeshes->DrawBoxMesh();

	// PS5 right Panel 
	SetShaderMaterial("PS5Material");
	scaleXYZ = glm::vec3(0.03f, 1.7f, 0.7f); 
	positionXYZ = glm::vec3(-1.85f, 2.26f, 0.25f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("Whitetex");
	m_basicMeshes->DrawBoxMesh(); 

	// PS5 Stand 
	SetShaderMaterial("PS5Material");
	scaleXYZ = glm::vec3(0.3f, 0.05f, 0.3f); 
	positionXYZ = glm::vec3(-2.0f, 1.6f, 0.25f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("Blackgloss");
	m_basicMeshes->DrawCylinderMesh();


	




	/*** Monitor Shapes ************************************************/
	/******************************************************************/

	// Monitor Bezel
	SetShaderMaterial("MonitorMaterial"); // Shader material set for monitor 
	scaleXYZ = glm::vec3(3.0f, 1.8f, 0.1f);
	positionXYZ = glm::vec3(0.0f, 2.7f, 0.0f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//Texture overlap for monitor bezel
	SetShaderTexture("stainless");
	SetShaderTexture("stainlessend");
	m_basicMeshes->DrawBoxMesh();

	// Monitor screen using box shape
	SetShaderMaterial("MonitorMaterial"); 
	scaleXYZ = glm::vec3(2.8f, 1.6f, 0.1f);
	positionXYZ = glm::vec3(0.0f, 2.7f, 0.05f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//image of Galaga for the monitor screen.
	SetShaderTexture("game");
	m_basicMeshes->DrawBoxMesh();

	// Stand base using box shape
	SetShaderMaterial("StandMaterial");//Shader material for stand
	scaleXYZ = glm::vec3(1.0f, 0.1f, 0.5f);
	positionXYZ = glm::vec3(0.0f, 1.6f, -0.14f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("stainless");
	m_basicMeshes->DrawBoxMesh();

	// Stand support using cylinder shape
	SetShaderMaterial("StandMaterial");
	scaleXYZ = glm::vec3(0.1f, 0.9f, 0.1f); 
	positionXYZ = glm::vec3(0.0f, 1.6f, -0.14f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("stainless");
	m_basicMeshes->DrawCylinderMesh();


	/*** White Speaker Shape *****************************************/
   /******************************************************************/

   // Speaker Base 
	SetShaderMaterial("SpeakerMaterial");//Shader material for Speaker
	scaleXYZ = glm::vec3(0.2f, 0.05f, 0.2f); 
	positionXYZ = glm::vec3(-1.3f, 1.63f, 0.3f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("Whitemarb");
	m_basicMeshes->DrawCylinderMesh();

	// Speaker Top
	SetShaderMaterial("SpeakerMaterial");
	scaleXYZ = glm::vec3(0.2f, 0.15f, 0.2f);
	positionXYZ = glm::vec3(-1.3f, 1.73f, 0.3f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("Whitemarb");
	m_basicMeshes->DrawSphereMesh();


	/*** Desk Shapes ************************************************/
	/******************************************************************/

	// Desk top using box shape
	SetShaderMaterial("DeskMaterial");//Shader material set for desk objects
	scaleXYZ = glm::vec3(6.0f, 0.2f, 2.5f); 
	positionXYZ = glm::vec3(0.0f, 1.5f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Texture set for plank
	SetShaderTexture("wood");
	m_basicMeshes->DrawBoxMesh();

	// Desk legs using box shape
	SetShaderMaterial("DeskMaterial");
	scaleXYZ = glm::vec3(0.2f, 1.5f, 0.2f); 
	// Front left leg
	positionXYZ = glm::vec3(-2.8f, 0.75f, 1.2f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Texture set desk legs 
	SetShaderTexture("wood");
	m_basicMeshes->DrawBoxMesh();

	// Front right leg
	SetShaderMaterial("DeskMaterial");
	positionXYZ = glm::vec3(2.8f, 0.75f, 1.2f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Texture set for right leg
	SetShaderTexture("wood");
	m_basicMeshes->DrawBoxMesh();

	// Back left leg
	SetShaderMaterial("DeskMaterial");
	positionXYZ = glm::vec3(-2.8f, 0.75f, -1.2f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Texture set for back left leg
	SetShaderTexture("wood");
	m_basicMeshes->DrawBoxMesh();

	// Back right leg
	SetShaderMaterial("DeskMaterial");
	positionXYZ = glm::vec3(2.8f, 0.75f, -1.2f); 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	// Texture set forback right leg
	SetShaderTexture("wood");
	m_basicMeshes->DrawBoxMesh();


	

	
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("floor");
	SetShaderMaterial("ReflectPlane");//Set shader to reflect light off plane. 
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
}