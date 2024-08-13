#include "modules/Starter.hpp"
#define ROAD_DIM 5

//Global
// Direct Light
struct GlobalUniformBufferObject {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 viewerPosition;
};


//Car uniform object
struct UniformBufferObject {
	alignas(16) glm::mat4 mvpMat; //World View Projection Matrix
	alignas(16) glm::mat4 mMat;   //Model/World Matrix
	alignas(16) glm::mat4 nMat;   //Normal Matrix
};

//Floor uniform objects
struct RoadUniformBufferObject {
	alignas(16) glm::mat4 mvpMat[2 * ROAD_DIM];
	alignas(16) glm::mat4 mMat[2 * ROAD_DIM];
	alignas(16) glm::mat4 nMat[2 * ROAD_DIM];
};


//Skybox
struct skyBoxUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
};

struct skyBoxVertex {
	glm::vec3 pos;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 normal;
};


// MAIN ! 
class A10 : public BaseProject {
protected:
	//Global
	DescriptorSetLayout DSLGlobal;
	DescriptorSet DSGlobal;

	//Skybox
	DescriptorSetLayout DSLSkyBox;
	VertexDescriptor VDSkyBox;
	Pipeline PSkyBox;
	Model MSkyBox;
	Texture TSkyBox, TStars;
	DescriptorSet DSSkyBox;

	// Road
	DescriptorSetLayout DSLroad;
	VertexDescriptor VDenv; 
	Pipeline Proad;
	Model MstraightRoad;
	Model MturnLeft;
	Model MturnRight;
	DescriptorSet DSstraightRoad;
	DescriptorSet DSturnLeft;
	DescriptorSet DSturnRight;

	// Car
	DescriptorSetLayout DSLcar; 
	VertexDescriptor VDcar; 
	Pipeline Pcar; 
	Model Mcar;
	DescriptorSet DScar; 

	// Environment
	Texture Tenv;


	float ar;
	float FOVy = glm::radians(60.0f);
	float nearPlane = 0.1f;
	float farPlane = 500.0f;

	/******* CAMERA PARAMETERS *******/
	float alpha = M_PI;					// yaw
	float beta = glm::radians(5.0f);    // pitch
	//static float rho = 0.0f;			// roll
	float camDist = 7.0f;				// distance from the target
	float camHeight = 2.0f;				// height from the target
	const float lambdaCam = 10.0f;      // damping factor for the camera

	glm::vec3 camPos = glm::vec3(0.0, camHeight, camDist);					//Camera Position (-l/+r, -d/+u, b/f)
	glm::mat4 viewMatrix = glm::translate(glm::mat4(1), -camPos);		//View Matrix setup
	glm::vec3 upVector = glm::vec3(0, 1, 0);							//Up Vector

	/******* CAR PARAMETERS *******/
	glm::vec3 startingCarPos = glm::vec3(0.0f);
	glm::vec3 updatedCarPos = glm::vec3(0.0f);
	const float ROT_SPEED = glm::radians(120.0f);
	const float MOVE_SPEED = 2.0f;
	const float carAcceleration = 8.0f;						// [m/s^2]
	const float carDeceleration = 4.0f; 
	const float attrition = 0.7f; 
	const float carSteeringSpeed = glm::radians(10.0f);
	const float carDampingSpeed = 1.5f;
	const float braking = 6.0f; 
	float steeringAng = 0.0f;
	float carVelocity = 0.0f;

	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "CG_PRJ";
    	windowResizable = GLFW_TRUE;
		
		ar = (float)windowWidth / (float)windowHeight;
	}
	
	// Window resize callback
	void onWindowResize(int w, int h) {
		ar = (float)w / (float)h;
	}
	
	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		//----------------Descriptor Set Layout----------------
		//Global
		DSLGlobal.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
			});

		//Skybox
		DSLSkyBox.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(skyBoxUniformBufferObject), 1},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
					{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1}
			});

		//Road
		DSLroad.init(this, {
				{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
				{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(RoadUniformBufferObject), 1}
		});

		//Car
		DSLcar.init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObject), 1},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
		});

		//----------------Vertex Descriptor----------------
		//Skybox
		VDSkyBox.init(this, {
				{0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
			}, {
				{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos), sizeof(glm::vec3), POSITION}
			});

		//Environment
		VDenv.init(this, {
				{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
			}, {
				{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
				{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv), sizeof(glm::vec2), UV},
				{0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal), sizeof(glm::vec3), NORMAL},
			});

		//Car
		VDcar.init(this, {
				{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
			}, {
				{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
				{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv), sizeof(glm::vec2), UV},
				{0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal), sizeof(glm::vec3), NORMAL},
			});


		//----------------Pipelines----------------
		PSkyBox.init(this, &VDSkyBox, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", {&DSLSkyBox});
		PSkyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false); 
		Proad.init(this, &VDenv, "shaders/EnvVert.spv", "shaders/EnvFrag.spv", {&DSLGlobal, &DSLroad});
		Proad.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Pcar.init(this, &VDcar, "shaders/CarVert.spv", "shaders/CarFrag.spv", {&DSLGlobal, &DSLcar}); 

		//----------------Models----------------
		MSkyBox.init(this, &VDSkyBox, "models/SkyBoxCube.obj", OBJ);
		Mcar.init(this, &VDcar, "models/car.mgcg", MGCG);
		MstraightRoad.init(this, &VDenv, "models/road/straight.mgcg", MGCG);
		MturnLeft.init(this, &VDenv, "models/road/turn.mgcg", MGCG);
		MturnRight.init(this, &VDenv, "models/road/turn.mgcg", MGCG);


		//----------------Textures----------------
		TSkyBox.init(this, "textures/starmap_g4k.jpg");
		Tenv.init(this, "textures/Textures_City.png"); 
		TStars.init(this, "textures/constellation_figures.png");

		DPSZs.uniformBlocksInPool = 6;				//# of uniform buffers  (Global, SkyBox, Car, Road * 3)
		DPSZs.texturesInPool = 6;					//# of textures (SkyBox, Stars, Car, Road * 3)
		DPSZs.setsInPool = 6;  						//# of DS (Global, SkyBox, Car, Road * 3)
		
		std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
		std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
		std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		//Descriptor Set initialization
		DSSkyBox.init(this, &DSLSkyBox, {&TSkyBox, &TStars});
		DSGlobal.init(this, &DSLGlobal, {});
		DSstraightRoad.init(this, &DSLroad, { &Tenv });
		DSturnLeft.init(this, &DSLroad, { &Tenv });
		DSturnRight.init(this, &DSLroad, { &Tenv });
		DScar.init(this, &DSLcar, {&Tenv});  
		
		//Pipeline Creation
		PSkyBox.create();
		Proad.create();
		Pcar.create();
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		//Pipelines Cleanup
		PSkyBox.cleanup();
		Proad.cleanup();
		Pcar.cleanup();

		//Descriptor Set Cleanup
		DSGlobal.cleanup();
		DSSkyBox.cleanup();
		DSstraightRoad.cleanup();
		DSturnLeft.cleanup();
		DSturnRight.cleanup();
		DScar.cleanup();
		
	}

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two different
	// methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup() {	
		//Textures Cleanup
		TSkyBox.cleanup();
		TStars.cleanup();
		Tenv.cleanup(); 

		//Models Cleanup
		MSkyBox.cleanup();
		MstraightRoad.cleanup();
		MturnLeft.cleanup();
		MturnRight.cleanup();
		Mcar.cleanup(); 

		//Descriptor Set Layouts Cleanup
		DSLGlobal.cleanup();
		DSLSkyBox.cleanup();
		DSLroad.cleanup();
		DSLcar.cleanup();
		
		//Pipelines destruction
		PSkyBox.destroy();
		Proad.destroy();
		Pcar.destroy();
	}
	
	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		//Draw SkyBox
		PSkyBox.bind(commandBuffer);
		MSkyBox.bind(commandBuffer);
		DSSkyBox.bind(commandBuffer, PSkyBox, 0, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MSkyBox.indices.size()), 1, 0, 0, 0);

		//Draw Car
		Pcar.bind(commandBuffer); 
		Mcar.bind(commandBuffer); 
		DSGlobal.bind(commandBuffer, Pcar, 0, currentImage); 
		DScar.bind(commandBuffer, Pcar, 1, currentImage); 
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mcar.indices.size()), 1, 0, 0, 0);

		Proad.bind(commandBuffer);

		MstraightRoad.bind(commandBuffer);
		DSstraightRoad.bind(commandBuffer, Proad, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MstraightRoad.indices.size()), 2 * ROAD_DIM, 0, 0, 0);

		MturnLeft.bind(commandBuffer);
		DSturnLeft.bind(commandBuffer, Proad, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MturnLeft.indices.size()), 1, 0, 0, 0);


		MturnRight.bind(commandBuffer);
		DSturnRight.bind(commandBuffer, Proad, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MturnRight.indices.size()), 1, 0, 0, 0);

	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		// Parameters for the SixAxis
		static glm::vec3 dampedCamPos = camPos;
		float deltaT;					// Time between frames [seconds]
		glm::vec3 m = glm::vec3(0.0f);  // Movement
		glm::vec3 r = glm::vec3(0.0f);  // Rotation
		bool fire = false;				// Button pressed
		getSixAxis(deltaT, m, r, fire); 

		//Matrices setup 
		glm::mat4 pMat = glm::perspective(FOVy, ar, nearPlane, farPlane);	//Projection Matrix
		pMat[1][1] *= -1;													//Flip Y
		glm::mat4 vpMat;													//View Projection Matrix

		/************************************* MOTION OF THE CAR *************************************/

		if (glfwGetKey(window, GLFW_KEY_SPACE)) {
			carVelocity -= braking * deltaT;
		}

		if (m.z < 0) { // w pressed
			carVelocity += carAcceleration * deltaT; 
			carVelocity = glm::min(carVelocity, 89.0f); 
		}
		if (m.z > 0) { // s pressed
			m.x *= -1; 
			if (carVelocity <= 0) {
				carVelocity -= carDeceleration * deltaT; 
				carVelocity = glm::max(carVelocity, -35.0f);
			}
		}
		else {
			carVelocity -= attrition * 9.81 * deltaT; 
			carVelocity = glm::max(carVelocity, 0.0f);
		}
		
		steeringAng += -m.x * carSteeringSpeed * deltaT;

		std::cout << "COS :" << glm::cos(steeringAng) << std::endl;
		std::cout << "SIN :" << glm::sin(steeringAng) << std::endl;

		/*steeringAng = (steeringAng < glm::radians(-35.0f) ? glm::radians(-35.0f) :
			(steeringAng > glm::radians(35.0f) ? glm::radians(35.0f) : steeringAng));*/

		startingCarPos.z -= carVelocity * deltaT * glm::cos(steeringAng);
		startingCarPos.x -= carVelocity * deltaT * glm::sin(steeringAng);
		updatedCarPos.z = updatedCarPos.z * std::exp(-carDampingSpeed * deltaT) + startingCarPos.z * (1 - std::exp(-carDampingSpeed * deltaT));
		updatedCarPos.x = updatedCarPos.x * std::exp(-carDampingSpeed* deltaT) + startingCarPos.x * (1 - std::exp(-carDampingSpeed * deltaT));	

		/************************************************************************************************/
		
		/************************************* Walk model procedure *************************************/
		// Walk model procedure
		//glm::vec3 ux = glm::vec3(glm::rotate(glm::mat4(1), alpha, glm::vec3(0,1,0)) * glm::vec4(1,0,0,1));
		//glm::vec3 uz = glm::vec3(glm::rotate(glm::mat4(1), alpha, glm::vec3(0,1,0)) * glm::vec4(0,0,1,1));
		alpha += ROT_SPEED * r.y * deltaT;	 // yaw, += for mouse movement
		beta -= ROT_SPEED * r.x * deltaT; // pitch
		//rho -= ROT_SPEED * r.z * deltaT;  // roll (not used)
		camDist -= MOVE_SPEED * deltaT * m.y;
		/*camPos -= ux * MOVE_SPEED * m.x * deltaT;
		camPos -= uz * MOVE_SPEED * m.z * deltaT; */
		
		beta = (beta < 0.0f ? 0.0f : (beta > M_PI_2 - 0.4f ? M_PI_2 - 0.4f : beta));	  // -0.3f to avoid camera flip for every camera distance
		camDist = (camDist < 5.0f ? 5.0f : (camDist > 15.0f ? 15.0f : camDist));	      // Camera distance limits
		
		camPos = updatedCarPos + glm::vec3(-glm::rotate(glm::mat4(1), alpha+steeringAng, glm::vec3(0, 1, 0)) *
										   glm::rotate(glm::mat4(1), beta, glm::vec3(1, 0, 0)) *
										   glm::vec4(0, -camHeight, camDist, 1));
		dampedCamPos = camPos * (1 - exp(-lambdaCam * deltaT)) +
						 dampedCamPos * exp(-lambdaCam * deltaT);

		viewMatrix = glm::lookAt(camPos, updatedCarPos, upVector);
		vpMat = pMat * viewMatrix; 
		/************************************************************************************************/

		//Update global uniforms				
		//Global
		GlobalUniformBufferObject g_ubo{};
		g_ubo.lightDir = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
		//g_ubo.lightDir = glm::vec3(0.0f, 10.0f, -20.0f);
		g_ubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		g_ubo.viewerPosition = glm::vec3(glm::inverse(viewMatrix) * glm::vec4(0, 0, 0, 1));
		DSGlobal.map(currentImage, &g_ubo, 0);
		
		//Object Uniform Buffer creation
		//SkyBox
		skyBoxUniformBufferObject sb_ubo{};
		sb_ubo.mvpMat = pMat * glm::mat4(glm::mat3(viewMatrix)); //Remove Translation part of ViewMatrix, take only Rotation part and applies Projection
		DSSkyBox.map(currentImage, &sb_ubo, 0);

		//Car
		UniformBufferObject car_ubo{}; 
		car_ubo.mMat = glm::translate(glm::mat4(1.0f), updatedCarPos) *
					   glm::rotate(glm::mat4(1.0f), glm::radians(180.0f) + steeringAng, glm::vec3(0, 1, 0)); 
		car_ubo.mvpMat = vpMat * car_ubo.mMat;
		car_ubo.nMat = glm::inverse(glm::transpose(car_ubo.mMat));
		DScar.map(currentImage, &car_ubo, 0);

		int i = 0;

		//Road
		RoadUniformBufferObject straight_road_ubo{};
		for (i = 0; i < 2 * ROAD_DIM; i++) {


			straight_road_ubo.mMat[i] = glm::mat4(1.0f);

			if (i >= ROAD_DIM) {
				straight_road_ubo.mMat[i] = glm::translate(glm::mat4(1.0f), glm::vec3(16.0f * ((i - ROAD_DIM)+ 1), 0.0f, -16.0f * ROAD_DIM)) *
					glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 1, 0));
			}
			else {
				straight_road_ubo.mMat[i] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -16.0f * i)) *
					glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
			}

			straight_road_ubo.mvpMat[i] = vpMat * straight_road_ubo.mMat[i];
			straight_road_ubo.nMat[i] = glm::inverse(glm::transpose(straight_road_ubo.mMat[i]));
		}
		DSstraightRoad.map(currentImage, &straight_road_ubo, 1);


		RoadUniformBufferObject turn_right{};
		turn_right.mMat[0] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -16.0f * ROAD_DIM)) *
				glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
		turn_right.mvpMat[0] = vpMat * turn_right.mMat[0];
		turn_right.nMat[0] = glm::inverse(glm::transpose(turn_right.mMat[0]));;
		DSturnRight.map(currentImage, &turn_right, 1);

		i++;

		RoadUniformBufferObject turn_left{};
		turn_left.mMat[0] = glm::translate(glm::mat4(1.0f), glm::vec3(11.0f, 0.0f, 50.0f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
		turn_left.mvpMat[0] = vpMat * turn_left.mMat[0];
		turn_left.nMat[0] = glm::inverse(glm::transpose(turn_left.mMat[0]));;
		DSturnLeft.map(currentImage, &turn_left, 1);

	}
};

// This is the main: probably you do not need to touch this!
int main() {
    A10 app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
