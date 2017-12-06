/**
 * Base code
 * Draws two meshes and one ground plane, one mesh has textures, as well
 * as ground plane.
 * Must be fixed to load in mesh with multiple shapes (goal.obj)
 */

#include <iostream>
#include <glad/glad.h>
#include "stb_image.h"

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "WindowManager.h"
#include "GLTextureWriter.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

class GameObject
{
	public:
	    // Object state
	    vec3 Position, Size, Velocity, Acceleration;
	    float Radius, RotX = 0.f, RotY = 0.f, RotZ = 0.f;
	    float currentSpeed = 0.f;
	    float currentTurnSpeed = 0.f;
	    float deltaX, deltaZ;
	    
	    // Constructor(s)
	    GameObject() {

	    }

	    GameObject(vec3 pos, vec3 size, vec3 velocity = vec3(1.0f, 1.0f, 1.0f), vec3 acceleration = vec3(1.0f, 1.0f, 1.0f), float radius = 0.0, float rotX = 0.0, float rotY = 0.0, float rotZ = 0.0) {
	    	Position = pos;
	    	Size = size;
	    	Velocity = velocity;
	    	Acceleration = acceleration;
	    	Radius = radius;
	    	RotX = rotX;
	    	RotY = rotY;
	    	RotZ = rotZ;
	    }

	    // Methods(s)

	    vec3 Move() {
	    	// DisplayManager.getFrameTimeSeconds()
		    IncreaseRotation(0, this->currentTurnSpeed, 0);
		    
		    float distance = this->currentSpeed * .01;

		    float dx = distance * cos(radians(this->RotY));
		    float dz = distance * sin(radians(this->RotY));

		    this->deltaX = dx;
		    this->deltaZ = dz;

		    IncreasePosition(dx, 0.f, dz);

		    // returning the position to set the position for foot
		    return this->Position;
        }

        void IncreaseRotation(float rotdx, float rotdy, float rotdz) {
        	this->RotX += rotdx;
        	this->RotY += rotdy;
        	this->RotZ += rotdz;
        }

        void IncreasePosition(float dx, float dy, float dz) {
        	this->Position.x += dx;
        	this->Position.y += dy;
        	this->Position.z += dz;
        }
};

void create_cube_map(
	  const char* front,
	  const char* back,
	  const char* top,
	  const char* bottom,
	  const char* left,
	  const char* right,
	  GLuint* tex_cube);

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog;
	std::shared_ptr<Program> texProg;
	std::shared_ptr<Program> texProg1;
	std::shared_ptr<Program> texProg2;

	// Shapes to be used (from obj file)
	std::vector<shared_ptr<Shape>> GoalShapes;
	std::vector<shared_ptr<Shape>> DummyShapes;
	//meshes with just one shape
	shared_ptr<Shape> world;
	shared_ptr<Shape> goal;
	shared_ptr<Shape> dummy;
	shared_ptr<Shape> dummyRightFoot;
	shared_ptr<Shape> cabin;

	//ground plane info
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int gGiboLen;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our triangle to OpenGL
	GLuint VertexBufferID;

	//geometry for texture render
	GLuint quad_VertexArrayID;
	GLuint quad_vertexbuffer;

	//three different textures
	shared_ptr<Texture> texture0;
 	shared_ptr<Texture> texture1;
 	shared_ptr<Texture> texture2;

	int gMat = 0;

	//For each shape, now that they are not resized, they need to be
	//transformed appropriately to the origin and scaled
	//transforms for each goal shape
	vec3 gGoalTrans = vec3(0);
	float gGoalScale = 1.0;

	vec3 gDummyTrans = vec3(0);
	float gDummyScale = 1.0;

	float limbRot = 0.0;
	bool leftArmUp = false;

	float ballXRot = 0.0;
	float ballZRot = 0.0;

	//transforms for the world
	vec3 gDTrans = vec3(0);
	float gDScale = 1.0;


	float cTheta = 0;
	float sTheta = 0;
	float leftTheta = 0;

	bool mouseDown = false;

	double currX, currY, lastX, lastY;
	// theta is for yaw, phi is for pitch
	double theta = -(M_PI / 2), phi = 0.0f;

	bool FirstTime = true;
	bool Moving = false;
	bool firstMouse = true;

	vec3 eyeVector = vec3(0.0f, 0.0f, 0.0f);
	vec3 lookAtVector = vec3(0.0f, 0.0f, -1.0f);
	vec3 upVector = vec3(0.0f, 1.0f, 0.0f);

	vec3 viewVector = lookAtVector - eyeVector;
    vec3 strafe = cross(viewVector, upVector);
    vec3 speed = vec3(.25, .25, .25);

    float lightPos[3] = {-6.0, 5.0, 0.0};

    // Variable for moving the light position
	float lightTrans = 0;

	// Dummy translation
	float dummyXTrans = 1.0;
	float dummyZTrans = -1.0;
	bool dummyMoving = false;

	vec3 ballTranslation = vec3(2, -.7f, -5);

	GameObject *Ball = new GameObject();
	GameObject *Player = new GameObject();
	GameObject *Foot = new GameObject();

	const float RUN_SPEED = 10.f;
	const float TURN_SPEED = 10.f;

	GLuint vbo;
	GLuint vao;
	GLuint texture_cube;
	GLuint* cube_texture = &texture_cube;

	bool changeCubeTexture = false;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		else if (key == GLFW_KEY_W && action == GLFW_REPEAT)
		{
            viewVector = lookAtVector - eyeVector;          
            eyeVector += (speed * viewVector);
            lookAtVector += (speed * viewVector);
        }
        else if (key == GLFW_KEY_S && action == GLFW_REPEAT)
        {
            viewVector = lookAtVector - eyeVector;          
            eyeVector -= (speed * viewVector);
            lookAtVector -= (speed * viewVector);
        }
        else if (key == GLFW_KEY_A && action == GLFW_REPEAT)
        {
            viewVector = lookAtVector - eyeVector;                          
            strafe = cross(viewVector, upVector);         
            eyeVector -= (speed * strafe);
            lookAtVector -= (speed * strafe);    
        }
        else if (key == GLFW_KEY_D && action == GLFW_REPEAT)
        {
            viewVector = lookAtVector - eyeVector;                          
            strafe = cross(viewVector, upVector);
            eyeVector += (speed * strafe);
            lookAtVector += (speed * strafe);
        }
		else if (key == GLFW_KEY_E && action == GLFW_PRESS) 
		{
			lightTrans += .25;

			lightPos[0] = lightTrans;
		} 
		else if (key == GLFW_KEY_Q && action == GLFW_PRESS) 
		{
			lightTrans -= .25;

			lightPos[0] = lightTrans;
		}
		else if (key == GLFW_KEY_I && action == GLFW_REPEAT) 
		{
			dummyMoving = true;

			Player->currentSpeed = RUN_SPEED;

			Foot->Position = Player->Move();
		}
		else if (key == GLFW_KEY_I && action == GLFW_RELEASE) 
		{
			dummyMoving = false;

			Player->currentSpeed = 0.f;
		}
		else if (key == GLFW_KEY_K && action == GLFW_REPEAT) 
		{
			dummyMoving = true;

			Player->currentSpeed = -RUN_SPEED;

			Foot->Position = Player->Move();
		}
		else if (key == GLFW_KEY_K && action == GLFW_RELEASE) 
		{
			dummyMoving = false;

			Player->currentTurnSpeed = 0.f;
		}
		else if (key == GLFW_KEY_J && action == GLFW_REPEAT) 
		{
			dummyMoving = true;

			Player->currentTurnSpeed = -TURN_SPEED;

			Foot->Position = Player->Move();
		}
		else if (key == GLFW_KEY_J && action == GLFW_RELEASE) 
		{
			dummyMoving = false;

			Player->currentTurnSpeed = 0.f;
		}
		else if (key == GLFW_KEY_L && action == GLFW_REPEAT) 
		{
			dummyMoving = true;

			Player->currentTurnSpeed = TURN_SPEED;

			Foot->Position = Player->Move();
		}
		else if (key == GLFW_KEY_L && action == GLFW_RELEASE) 
		{
			Player->currentTurnSpeed = 0.f;

			dummyMoving = false;
		}
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY)
	{
		cTheta += (float) deltaX;
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			Moving = true;
			mouseDown = true;

			glfwGetCursorPos(window, &currX, &currY);

			lastX = currX;
			lastY = currY;
		}

		if (action == GLFW_RELEASE)
		{
			Moving = false;
			mouseDown = false;

			glfwGetCursorPos(window, &currX, &currY);
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	// Code to load in the three textures
	void initTex(const std::string& resourceDirectory)
	{
	 	texture0 = make_shared<Texture>();
		texture0->setFilename(resourceDirectory + "/soccer_field.jpg");
		texture0->init();
		texture0->setUnit(0);
		texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		texture1 = make_shared<Texture>();
		texture1->setFilename(resourceDirectory + "/soccer_texture.jpg");
		texture1->init();
		texture1->setUnit(1);
		texture1->setWrapModes(GL_REPEAT, GL_REPEAT);

		texture2 = make_shared<Texture>();
		texture2->setFilename(resourceDirectory + "/mars.jpg");
		texture2->init();
		texture2->setUnit(2);
		texture2->setWrapModes(GL_REPEAT, GL_REPEAT);
	}

	//code to set up the two shaders - a diffuse shader and texture mapping
	void init(const std::string& resourceDirectory)
	{
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		GLSL::checkVersion();

		cTheta = 0;
		// Set background color.
		glClearColor(.12f, .34f, .56f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(
			resourceDirectory + "/simple_vert.glsl",
			resourceDirectory + "/simple_frag.glsl");
		if (! prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->addUniform("P");
		prog->addUniform("M");
		prog->addUniform("V");
		prog->addUniform("lightPos");
		prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		prog->addUniform("MatSpec");
		prog->addUniform("shine");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");

		//initialize the textures we might use
		initTex(resourceDirectory);

		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(
			resourceDirectory + "/tex_vert.glsl",
			resourceDirectory + "/tex_frag0.glsl");
		if (! texProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
 		texProg->addUniform("P");
		texProg->addUniform("M");
		texProg->addUniform("V");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");
		texProg->addUniform("Texture0");

		texProg1 = make_shared<Program>();
		texProg1->setVerbose(true);
		texProg1->setShaderNames(
			resourceDirectory + "/tex_vert.glsl",
			resourceDirectory + "/tex_frag1.glsl");
		if (! texProg1->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
 		texProg1->addUniform("P");
		texProg1->addUniform("M");
		texProg1->addUniform("V");
		texProg1->addAttribute("vertPos");
		texProg1->addAttribute("vertNor");
		texProg1->addAttribute("vertTex");
		texProg1->addUniform("Texture0");

		// cube map
		texProg2 = make_shared<Program>();
		texProg2->setVerbose(true);
		texProg2->setShaderNames(
			resourceDirectory + "/cube_map_vert.glsl",
			resourceDirectory + "/cube_map_frag.glsl");
		if (! texProg2->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
 		texProg2->addUniform("P");
		texProg2->addUniform("M");
		texProg2->addUniform("V");
		texProg2->addAttribute("vertTex");

		create_cube_map((resourceDirectory + "/sincity_ft.tga").c_str(), (resourceDirectory + "/sincity_bk.tga").c_str(), (resourceDirectory + "/sincity_up.tga").c_str(), 
			(resourceDirectory + "/sincity_dn.tga").c_str(), (resourceDirectory + "/sincity_lf.tga").c_str(), (resourceDirectory + "/sincity_rt.tga").c_str(), cube_texture);
	}

	void initGeom(const std::string& resourceDirectory)
	{
		// Load geometry
		// Some obj files contain material information.
		// We'll ignore them for this assignment.
		// this is the tiny obj shapes - not to be confused with our shapes
		vector<tinyobj::shape_t> TOshapes;
		vector<tinyobj::material_t> objMaterials;

		string errStr;
		//load in the mesh and make the shapes
		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr,
						(resourceDirectory + "/tinker.obj").c_str());

		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			// some data to keep track of where our mesh is in space
			vec3 minGoalVec, maxGoalVec;
			minGoalVec = vec3(std::numeric_limits<float>::max());
			maxGoalVec = vec3(-std::numeric_limits<float>::max());

			for (size_t i = 0; i < TOshapes.size(); i++)
			{
				// TODO -- Initialize each mesh
				// 1. make a shared pointer
				// 2. createShape for each tiny obj shape
				// 3. measure each shape to find out its AABB
				// 4. call init on each shape to create the GPU data
				// perform some record keeping to keep track of global min and max

				// Add the shape to AllShapes

				goal = make_shared<Shape>();
				goal->createShape(TOshapes[i]);
				goal->measure();
				goal->init();

				GoalShapes.push_back(goal);

				if (goal->max.x > maxGoalVec.x)
				{
					maxGoalVec.x = goal->max.x;
				}

				if (goal->max.y > maxGoalVec.y)
				{
					maxGoalVec.y = goal->max.y;
				}

				if (goal->max.z > maxGoalVec.z)
				{
					maxGoalVec.z = goal->max.z;
				}

				if (goal->min.x < minGoalVec.x)
				{
					minGoalVec.x = goal->min.x;
				}

				if (goal->min.y < minGoalVec.y)
				{
					minGoalVec.y = goal->min.y;
				}

				if (goal->min.z < minGoalVec.z)
				{
					minGoalVec.z = goal->min.z;
				}
			}

			// think about scale and translate....
			// based on the results of calling measure on each piece

			// compute its transforms based on measuring it
			gGoalTrans = minGoalVec + 0.5f * (maxGoalVec - minGoalVec);
			
			if (maxGoalVec.x > maxGoalVec.y && maxGoalVec.x > maxGoalVec.z)
			{
				gGoalScale = 2.0 / (maxGoalVec.x - minGoalVec.x);
			}
			else if (maxGoalVec.y > maxGoalVec.x && maxGoalVec.y > maxGoalVec.z)
			{
				gGoalScale = 2.0 / (maxGoalVec.y - minGoalVec.y);
			}
			else
			{
				gGoalScale = 2.0 / (maxGoalVec.z - minGoalVec.z);
			}
		}

		//load in the mesh and make the shapes
		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr,
						(resourceDirectory + "/dummy.obj").c_str());

		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			// some data to keep track of where our mesh is in space
			vec3 minDummyVec, maxDummyVec;
			minDummyVec = vec3(std::numeric_limits<float>::max());
			maxDummyVec = vec3(-std::numeric_limits<float>::max());

			for (size_t i = 0; i < TOshapes.size(); i++)
			{
				// TODO -- Initialize each mesh
				// 1. make a shared pointer
				// 2. createShape for each tiny obj shape
				// 3. measure each shape to find out its AABB
				// 4. call init on each shape to create the GPU data
				// perform some record keeping to keep track of global min and max

				// Add the shape to AllShapes

				dummy = make_shared<Shape>();
				dummy->createShape(TOshapes[i]);
				dummy->measure();
				dummy->init();

				DummyShapes.push_back(dummy);

				if (dummy->max.x > maxDummyVec.x)
				{
					maxDummyVec.x = dummy->max.x;
				}

				if (dummy->max.y > maxDummyVec.y)
				{
					maxDummyVec.y = dummy->max.y;
				}

				if (dummy->max.z > maxDummyVec.z)
				{
					maxDummyVec.z = dummy->max.z;
				}

				if (dummy->min.x < minDummyVec.x)
				{
					minDummyVec.x = dummy->min.x;
				}

				if (dummy->min.y < minDummyVec.y)
				{
					minDummyVec.y = dummy->min.y;
				}

				if (dummy->min.z < minDummyVec.z)
				{
					minDummyVec.z = dummy->min.z;
				}
			}

			// think about scale and translate....
			// based on the results of calling measure on each piece

			// compute its transforms based on measuring it
			gDummyTrans = minDummyVec + 0.5f * (maxDummyVec - minDummyVec);
			
			if (maxDummyVec.x > maxDummyVec.y && maxDummyVec.x > maxDummyVec.z)
			{
				gDummyScale = 2.0 / (maxDummyVec.x - minDummyVec.x);
			}
			else if (maxDummyVec.y > maxDummyVec.x && maxDummyVec.y > maxDummyVec.z)
			{
				gDummyScale = 2.0 / (maxDummyVec.y - minDummyVec.y);
			}
			else
			{
				gDummyScale = 2.0 / (maxDummyVec.z - minDummyVec.z);
			}
		}

		// now read in the sphere for the world
		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr,
						(resourceDirectory + "/sphere.obj").c_str());

		world = make_shared<Shape>();
		world->createShape(TOshapes[0]);
		world->measure();
		world->init();

		// compute its transforms based on measuring it
		gDTrans = world->min + 0.5f*(world->max - world->min);
		if (world->max.x >world->max.y && world->max.x > world->max.z)
		{
			gDScale = 2.0/(world->max.x-world->min.x);
		}
		else if (world->max.y > world->max.x && world->max.y > world->max.z)
		{
			gDScale = 2.0/(world->max.y-world->min.y);
		}
		else
		{
			gDScale = 2.0/(world->max.z-world->min.z);
		}


		vec3 ballCenter = vec3((world->min.x + (world->max.x - world->min.x)) / 2.0, (world->min.y + (world->max.y - world->min.y)) / 2.0, (world->min.z + (world->max.z - world->min.z)) / 2.0);

		ballCenter = ballCenter + vec3(2, -.7f, -5);

		Ball->Position.x = ballCenter.x;
		Ball->Position.y = ballCenter.y;
		Ball->Position.z = ballCenter.z;

		Ball->Velocity.x = 1.0;
		Ball->Velocity.y = 1.0;
		Ball->Velocity.z = 1.0;

		Ball->Radius = .5 * gDScale * .3;

		dummyRightFoot = DummyShapes[26];

		Foot->Position.x = gDummyScale * (dummyRightFoot->max.x - dummyRightFoot->min.x) / 2.0;
		Foot->Position.y = gDummyScale * (dummyRightFoot->max.y - dummyRightFoot->min.y) / 2.0;
		Foot->Position.z = gDummyScale * (dummyRightFoot->max.z - dummyRightFoot->min.z) / 2.0;

		// Initial translation
		Foot->Position.x += 1;
		Foot->Position.z -= 1;

		Foot->Radius = 2.0 * (gDummyScale * (dummyRightFoot->max.x - dummyRightFoot->min.x)) / 2.0; 

		// Initialize the geometry to render a ground plane
		initQuad();

		float points[] = {
		  -50.0f,  50.0f, -50.0f,
		  -50.0f, -50.0f, -50.0f,
		   50.0f, -50.0f, -50.0f,
		   50.0f, -50.0f, -50.0f,
		   50.0f,  50.0f, -50.0f,
		  -50.0f,  50.0f, -50.0f,
		  
		  -50.0f, -50.0f,  50.0f,
		  -50.0f, -50.0f, -50.0f,
		  -50.0f,  50.0f, -50.0f,
		  -50.0f,  50.0f, -50.0f,
		  -50.0f,  50.0f,  50.0f,
		  -50.0f, -50.0f,  50.0f,
		  
		   50.0f, -50.0f, -50.0f,
		   50.0f, -50.0f,  50.0f,
		   50.0f,  50.0f,  50.0f,
		   50.0f,  50.0f,  50.0f,
		   50.0f,  50.0f, -50.0f,
		   50.0f, -50.0f, -50.0f,
		   
		  -50.0f, -50.0f,  50.0f,
		  -50.0f,  50.0f,  50.0f,
		   50.0f,  50.0f,  50.0f,
		   50.0f,  50.0f,  50.0f,
		   50.0f, -50.0f,  50.0f,
		  -50.0f, -50.0f,  50.0f,
		  
		  -50.0f,  50.0f, -50.0f,
		   50.0f,  50.0f, -50.0f,
		   50.0f,  50.0f,  50.0f,
		   50.0f,  50.0f,  50.0f,
		  -50.0f,  50.0f,  50.0f,
		  -50.0f,  50.0f, -50.0f,
		  
		  -50.0f, -50.0f, -50.0f,
		  -50.0f, -50.0f,  50.0f,
		   50.0f, -50.0f, -50.0f,
		   50.0f, -50.0f, -50.0f,
		  -50.0f, -50.0f,  50.0f,
		   50.0f, -50.0f,  50.0f
		};

		
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 3 * 36 * sizeof(float), &points, GL_STATIC_DRAW);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	}

	/**** geometry set up for ground plane *****/
	void initQuad()
	{
		float g_groundSize = 20;
		float g_groundY = -1.5;

		// A x-z plane at y = g_groundY of dim[-g_groundSize, g_groundSize]^2
		float GrndPos[] = {
			-g_groundSize, g_groundY, -g_groundSize,
			-g_groundSize, g_groundY,  g_groundSize,
			 g_groundSize, g_groundY,  g_groundSize,
			 g_groundSize, g_groundY, -g_groundSize
		};

		float GrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		float GrndTex[] = {
			0, 0, // back
			0, 1,
			1, 1,
			1, 0
		};

		unsigned short idx[] = {0, 1, 2, 0, 2, 3};

		GLuint VertexArrayID;
		//generate the VAO
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);

		gGiboLen = 6;
		glGenBuffers(1, &GrndBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

		glGenBuffers(1, &GrndNorBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

		glGenBuffers(1, &GrndTexBuffObj);
		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

		glGenBuffers(1, &GIndxBuffObj);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
	}

	void create_cube_map(
	  const char* front,
	  const char* back,
	  const char* top,
	  const char* bottom,
	  const char* left,
	  const char* right,
	  GLuint* tex_cube) {
	  // generate a cube-map texture to hold all the sides
	  glActiveTexture(GL_TEXTURE0);
	  glGenTextures(1, tex_cube);
	  
	  // load each image and copy into a side of the cube-map texture
	  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, front);
	  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, back);
	  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, top);
	  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, bottom);
	  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, right);
	  load_cube_map_side(*tex_cube, GL_TEXTURE_CUBE_MAP_POSITIVE_X, left);
	  // format cube map texture
	  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	bool load_cube_map_side(
	  GLuint texture, GLenum side_target, const char* file_name) {
	  glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	  int x, y, n;
	  int force_channels = 4;
	  unsigned char*  image_data = stbi_load(
	    file_name, &x, &y, &n, force_channels);
	  if (!image_data) {
	    fprintf(stderr, "ERROR: could not load %s\n", file_name);
	    return false;
	  }

	  // non-power-of-2 dimensions check
	  if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
	    fprintf(stderr,
	    	"WARNING: image %s is not power-of-2 dimensions\n",
	    	file_name);
	  }
	  
	  // copy image data into 'target' side of cube map
	  glTexImage2D(
	    side_target,
	    0,
	    GL_RGBA,
	    x,
	    y,
	    0,
	    GL_RGBA,
	    GL_UNSIGNED_BYTE,
	    image_data);
	  free(image_data);

	  return true;
	}

	void renderGround()
	{
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		// draw!
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
		glDrawElements(GL_TRIANGLES, gGiboLen, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	void renderCubeMap() {
		glDepthMask(GL_FALSE);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, *cube_texture);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthMask(GL_TRUE);
	}

	void render()
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* Leave this code to just draw the meshes alone */
		float aspect = width/(float)height;

		// Create the matrix stacks
		auto P = make_shared<MatrixStack>();
		auto M = make_shared<MatrixStack>();
		auto V = make_shared<MatrixStack>();
		// Apply perspective projection.
		P->pushMatrix();
		P->perspective(45.0f, aspect, 0.01f, 100.0f);

		if (Moving)
		{
			if (firstMouse) {
				lastX = currX;
				lastY = currY;
				firstMouse = false;
			}

			double xoffset = currX - lastX;
			double yoffset = currY - lastY;
			lastX = currX;
			lastY = currY;

			float scale = 0.005;
			xoffset *= scale;
			yoffset *= scale;

			if (yoffset > 0) {
				if (phi + yoffset < 1.4) {
					phi += yoffset;
				}
			}
			else {
				if (phi + yoffset > -1.4) {
					phi += yoffset;
				}
			}

			theta += xoffset;

			lookAtVector.x = eyeVector.x + cos(theta) * cos(phi);
			lookAtVector.y = eyeVector.y + sin(phi);
			lookAtVector.z = eyeVector.z + cos(phi) * cos((3.14f / 2.0f) - theta);

			glfwGetCursorPos(windowManager->getHandle(), &currX, &currY);
		}

		//Draw our scene - two meshes and ground plane
		prog->bind();
			// Send light position
			glUniform3fv(prog->getUniform("lightPos"), 1, lightPos);
	   		// View matrix for the camera
		   	V->pushMatrix();
			   	V->loadIdentity();

			   	V->lookAt(eyeVector, lookAtVector, upVector);

			   	glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
			V->popMatrix();


			glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));

			// Draw first goal post
			M->pushMatrix();
				M->loadIdentity();
				M->rotate(radians(cTheta), vec3(0, 1, 0));

				M->translate(vec3(-6.0, .4, -1.9));
				
				M->rotate(radians(-90.f), vec3(0, 1, 0));
				M->scale(gGoalScale);
				//MV->translate(-1.0f * gGoalTrans);
				
				SetMaterial(2, prog);
				glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
				
				for (size_t i = 0; i < GoalShapes.size(); i++)
				{
					goal = GoalShapes[i];
					goal->draw(prog);
				}

			M->popMatrix();

			// Draw second goal post
			M->pushMatrix();
				M->loadIdentity();
				M->rotate(radians(cTheta), vec3(0, 1, 0));

				M->translate(vec3(16.0, .4, -1.9));
				
				M->rotate(radians(90.f), vec3(0, 1, 0));
				M->scale(gGoalScale);
				//MV->translate(-1.0f * gGoalTrans);
				
				SetMaterial(0, prog);
				glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
				
				for (size_t i = 0; i < GoalShapes.size(); i++)
				{
					goal = GoalShapes[i];
					goal->draw(prog);
				}

			M->popMatrix();

			// Draw dummy
			M->pushMatrix();
				M->loadIdentity();
				M->rotate(radians(cTheta), vec3(0, 1, 0));

				M->translate(vec3(Player->Position.x, -1.0, Player->Position.z));

				M->rotate(-radians(Player->RotY), vec3(0, 1, 0));
				
				M->rotate(radians(-90.f), vec3(1, 0, 0));
				
				SetMaterial(3, prog);

				//dummy model notes: 
			    //dummy is 29 shapes
			    //(from dummies perspective)
			    //left leg: 0-5 
			    //left arm: 6-11
			    //right arm: 12, 15, 18, 22, 27, 28
			    //right leg: 14, 16, 19, 20, 25, 26
			    //head and neck: 13, 17
			    //torso and pelvis: 21, 23, 24,

			    //12: right upper arm
			    //13: neck joint
			    //14: right upper leg
			    //15: right shoulder joint
			    //16: right knee joint
			    //17: head
			    //18: right elbow joint
			    //19: right lower leg
			    //20: right ankle joint
			    //21: torso
			    //22: right hand
			    //23: middle large pelvis joint
			    //24: pelvis joint cover
			    //25: right pelvis joint (part of leg)
			    //26: right foot
			    //27: right forearm
			    //28: right wrist joint

				//draw animated left arm (walking)
				M->pushMatrix();
		        
			        M->pushMatrix();

			            //move arm back to shoulder
                   		M->translate(vec3(0, -.57, 1.67));

			            //make the arm move
			            M->rotate(radians(-limbRot), vec3(0, 1, 0));

			            //put arm at side
			            M->rotate(radians(-75.f), vec3(1, 0, 0));

			            //move shoulder joint to the origin
  						M->translate(vec3(0, .1, -.85));

			            M->scale(gDummyScale);

			            glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));

			            for (size_t i = 6; i < 12; i++)
						{
							dummy = DummyShapes[i];
							dummy->draw(prog);
						}

			        M->popMatrix();

			        //draw animated right arm (walking)
	                M->pushMatrix();

	                    //move arm back to shoulder
                    	M->translate(vec3(0, .57, 1.67));

	                    //make the arm move
	                    M->rotate(radians(limbRot), vec3(0, 1, 0));

	                    //put arm at side
	                    M->rotate(radians(75.f), vec3(1, 0, 0));

	                    //move shoulder joint to the origin
	                    M->translate(vec3(0, -.1, -.85));

	                    M->scale(gDummyScale);

	                    glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));

	                    //right arm: 12, 15, 18, 22, 27, 28

	                    dummy = DummyShapes[12];
						dummy->draw(prog);

						dummy = DummyShapes[15];
						dummy->draw(prog);

						dummy = DummyShapes[18];
						dummy->draw(prog);
	                    
	                    dummy = DummyShapes[22];
						dummy->draw(prog);

						dummy = DummyShapes[27];
						dummy->draw(prog);

						dummy = DummyShapes[28];
						dummy->draw(prog);

	                M->popMatrix();

	                //draw animated left leg (walking)
	                M->pushMatrix();
	                    //move back to hip	                    
                    	M->translate(vec3(0, .07, 1.07));
	                    
	                    //rotate the hip joint
	                    M->rotate(radians(limbRot), vec3(0, 1, 0));
	    
	                    //move hip joint to origin
	                    M->translate(vec3(0, -.07, -1.05));

	                    M->scale(gDummyScale);

	                    glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));

	                    for (size_t i = 0; i < 6; i++)
						{
							dummy = DummyShapes[i];
							dummy->draw(prog);
						}
	                    
	                M->popMatrix();

	                //draw animated right leg (walking)
	                //right leg: 14, 16, 19, 20, 25, 26
	                M->pushMatrix();

	                    //move back to hip
	                    M->translate(vec3(0, -.07, 1.05));
	                    
	                    //rotate the hip joint
	                    M->rotate(radians(-limbRot), vec3(0, 1, 0));
	    
	                    //move hip joint to origin
	                    M->translate(vec3(0, .07, -1.05));
	                    M->scale(gDummyScale);

	                    glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));

	                    dummy = DummyShapes[14];
						dummy->draw(prog);

						dummy = DummyShapes[16];
						dummy->draw(prog);

						dummy = DummyShapes[19];
						dummy->draw(prog);
	                    
	                    dummy = DummyShapes[20];
						dummy->draw(prog);

						dummy = DummyShapes[25];
						dummy->draw(prog);

						dummy = DummyShapes[26];
						dummy->draw(prog);

	                M->popMatrix();

	                //render rest of the body
	                M->pushMatrix();	                
	                    M->scale(gDummyScale);
	                    glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));

	                    //head and neck: 13, 17
    					//torso and pelvis: 21, 23, 24
	                    dummy = DummyShapes[13];
						dummy->draw(prog);

						dummy = DummyShapes[17];
						dummy->draw(prog);

						dummy = DummyShapes[21];
						dummy->draw(prog);
	                    
	                    dummy = DummyShapes[23];
						dummy->draw(prog);

						dummy = DummyShapes[24];
						dummy->draw(prog);
	                M->popMatrix();

			    M->popMatrix();

			M->popMatrix();

		
		prog->unbind();

	
		texProg->bind();
			glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));

			// View matrix for the camera
		   	V->pushMatrix();
			   	V->loadIdentity();

			   	V->lookAt(eyeVector, lookAtVector, upVector);

			   	glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
			V->popMatrix();

			/* draw soccer ball */
			M->pushMatrix();
				M->loadIdentity();
				M->rotate(radians(cTheta), vec3(0, 1, 0));

				bool collision = CheckCollision(*Ball, *Foot);
				
				if (collision) {
					cout << "COLLISION" << endl << endl;

					Ball->currentSpeed = Player->currentSpeed;
					Ball->currentTurnSpeed = Player->currentTurnSpeed;

					Ball->Position.x += Player->deltaX;
					Ball->Position.z += Player->deltaZ;
				}


				/*
				if(ball_moving){
		         //h is the stepsize
		                  //ballVelocity is initialized to the view vector when first thrown
		         ballVelocity += + h/m * f;
		                  //ballPos is initialized to the eye vector when first thrown
		         ballPos += h * ballVelocity;

		                  //if the ball touches the ground, make it stop moving
		                  if(touchingGround(ballPos)){
		                      ball_moving = false;
		                      fetch = true;
		                  }
		      	}
		      */

				// add collision detection

				M->translate(vec3(Ball->Position.x, -.7, Ball->Position.z));

				M->scale(gDScale * .3);
				//M->translate(-1.0f * gDTrans);
				texture1->bind(texProg->getUniform("Texture0"));
				/*draw soccer ball*/
				glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));

				world->draw(texProg);
			M->popMatrix();
		texProg->unbind();

		texProg1->bind();
			glUniformMatrix4fv(texProg1->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));

			// View matrix for the camera
		   	V->pushMatrix();
			   	V->loadIdentity();

			   	V->lookAt(eyeVector, lookAtVector, upVector);

			   	glUniformMatrix4fv(texProg1->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
			V->popMatrix();

			M->pushMatrix();
				M->loadIdentity();
				M->rotate(radians(cTheta), vec3(0, 1, 0));
				M->pushMatrix();
					M->translate(vec3(5, 0.f, -2));
					M->scale(gDScale * .7);
					M->translate(-1.0f * gDTrans);
					texture2->bind(texProg1->getUniform("Texture0"));
				glUniformMatrix4fv(texProg1->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));

				/*draw the ground */
				glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
				texture0->bind(texProg->getUniform("Texture0"));
				renderGround();
			M->popMatrix();
		texProg1->unbind();

		texProg2->bind();
			glUniformMatrix4fv(texProg2->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));

			// View matrix for the camera
		   	V->pushMatrix();
			   	V->loadIdentity();

			   	V->lookAt(eyeVector, lookAtVector, upVector);

			   	glUniformMatrix4fv(texProg2->getUniform("V"), 1, GL_FALSE, value_ptr(V->topMatrix()));
			V->popMatrix();

			M->pushMatrix();
				glUniformMatrix4fv(texProg2->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
			M->popMatrix();

			renderCubeMap();
		texProg2->unbind();

		P->popMatrix();


		if (dummyMoving) {
			if (limbRot > 20) {
            	leftArmUp = false;
	        }
	        else if (limbRot < -20) {
	            leftArmUp = true;
	        }

	        if (leftArmUp == true) {
	            limbRot += .4;
	        }
	        else {
	            limbRot -= .4;
	        }
		}
		else {
			// Reset dummy to just standing
			limbRot = 0.0;
			leftArmUp = false;
		}
	}

	bool CheckCollision(GameObject &one, GameObject &two) // AABB - Circle collision
	{
	    float dx = one.Position.x - two.Position.x;
		float dy = one.Position.y - two.Position.y;
		float dz = one.Position.z - two.Position.z;

		float distance = sqrt(dx*dx + dy*dy + dz*dz);

		return distance <= (one.Radius + two.Radius);
	}

	// helper function to set materials for shading
	void SetMaterial(int i, std::shared_ptr<Program> prog)
	{
		switch (i)
		{
		case 0: //shiny blue plastic
			glUniform3f(prog->getUniform("MatAmb"), 0.02f, 0.04f, 0.2f);
			glUniform3f(prog->getUniform("MatDif"), 0.0f, 0.16f, 0.9f);
			glUniform3f(prog->getUniform("MatSpec"), 0.9922, 0.941176, 0.9);
			glUniform1f(prog->getUniform("shine"), 180);
			break;
		case 1: // flat grey
			glUniform3f(prog->getUniform("MatAmb"), 0.13f, 0.13f, 0.14f);
			glUniform3f(prog->getUniform("MatDif"), 0.3f, 0.3f, 0.4f);
			glUniform3f(prog->getUniform("MatSpec"), 0.9922, 0.941176, 0.9);
			glUniform1f(prog->getUniform("shine"), 180);
			break;
		case 2: //brass
			glUniform3f(prog->getUniform("MatAmb"), 0.3294f, 0.2235f, 0.02745f);
			glUniform3f(prog->getUniform("MatDif"), 0.7804f, 0.5686f, 0.11373f);
			glUniform3f(prog->getUniform("MatSpec"), 0.9922, 0.941176, 0.9);
			glUniform1f(prog->getUniform("shine"), 180);
			break;
		case 3: //copper
			glUniform3f(prog->getUniform("MatAmb"), 0.1913f, 0.0735f, 0.0225f);
			glUniform3f(prog->getUniform("MatDif"), 0.7038f, 0.27048f, 0.0828f);
			glUniform3f(prog->getUniform("MatSpec"), 0.9922, 0.941176, 0.9);
			glUniform1f(prog->getUniform("shine"), 180);
			break;
		case 4: // shiny chocolate
			glUniform3f(prog->getUniform("MatAmb"), 0.02, 0.20, 0.027);
			glUniform3f(prog->getUniform("MatDif"), 0.8, 0.16, 0.21);
			glUniform3f(prog->getUniform("MatSpec"), 0.9922, 0.941176, 0.9);
			glUniform1f(prog->getUniform("shine"), 180);
			break;
		case 5: // plastic pink
			glUniform3f(prog->getUniform("MatAmb"), 0.20, 0.02, 0.027);
			glUniform3f(prog->getUniform("MatDif"), 0.8, 0.16, 0.21);
			glUniform3f(prog->getUniform("MatSpec"), 0.9922, 0.941176, 0.9);
			glUniform1f(prog->getUniform("shine"), 180.0);
			break;
		case 6: // matte black
	 		glUniform3f(prog->getUniform("MatAmb"), 0.01, 0.01, 0.01);
	 		glUniform3f(prog->getUniform("MatDif"), 0.03, 0.03, 0.04);
	 		glUniform3f(prog->getUniform("MatSpec"), 0.2, 0.2, 0.2);
			glUniform1f(prog->getUniform("shine"), 1.0);
			break;
		}
	}

};

int main(int argc, char **argv)
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
			resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(512, 512);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
