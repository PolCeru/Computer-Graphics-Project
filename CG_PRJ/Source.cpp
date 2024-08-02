#include "modules/Starter.hpp"
#define STRAIGHT_ROAD_DIM 3

//Global
// Direct Light
struct GlobalUniformBufferObject {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 viewerPosition;
};


//Car uniform object
struct CarUniformBufferObject {
	alignas(16) glm::mat4 mvpMat; //World View Projection Matrix
	alignas(16) glm::mat4 mMat;   //Model/World Matrix
	alignas(16) glm::mat4 nMat;   //Normal Matrix
};

//Floor uniform objects
struct SraightRoadUniformBufferObject {
	alignas(16) glm::mat4 mvpMat[STRAIGHT_ROAD_DIM];
	alignas(16) glm::mat4 mMat[STRAIGHT_ROAD_DIM];
	alignas(16) glm::mat4 nMat[STRAIGHT_ROAD_DIM];
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

	//Environment 
	DescriptorSetLayout DSLenv; 
	VertexDescriptor VDenv; 
	Pipeline Penv; 
	Model Mfloor; 
	Texture Tstraight; 
	DescriptorSet DSenv; 

	//Car 
	DescriptorSetLayout DSLcar; 
	VertexDescriptor VDcar; 
	Pipeline Pcar; 
	Model Mcar;
	Texture Tcar; 
	DescriptorSet DScar; 

	//Application Parameters
	glm::vec3 camPos = glm::vec3(0.0, 2.0, 15.0);					//Camera Position (-l/+r, -d/+u, b/f)
	glm::mat4 ViewMatrix = glm::translate(glm::mat4(1), -camPos);   //View Matrix setup

	float Ar;
	float FOVy = glm::radians(60.0f);
	float nearPlane = 0.1f;
	float farPlane = 500.0f;

	/******* CAR PARAMETERS *******/
	glm::vec3 startingCarPos = glm::vec3(0.0f);
	glm::vec3 updatedCarPos = glm::vec3(0.0f);
	float steeringAng = 0.0f;
	float carVelocity = 0.0f;
	float carAcceleration = 1.0f;						// realistically it should depend on the acceleration of the car
	float carDeceleration = 0.0f;						// Not implemented the logic yer, but it should depend on the car brake
	float carDecelerationEffect = 10.0f;				//realistically it should depend on the surface attrition 
	float carSteeringSpeed = glm::radians(30.0f);
	float carDampingSpeed = 1.5f;
	
	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "CG_PRJ";
    	windowResizable = GLFW_TRUE;
		
		Ar = (float)windowWidth / (float)windowHeight;
	}
	
	// Window resize callback
	void onWindowResize(int w, int h) {
		Ar = (float)w / (float)h;
	}
	
	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		//----------------Descriptor Set Layout----------------
		//Global
		DSLGlobal.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1},
					{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(SraightRoadUniformBufferObject), 1},
			});


		//Skybox
		DSLSkyBox.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(skyBoxUniformBufferObject), 1},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
					{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1}
			});

		//Environment
		DSLenv.init(this, {
				{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
		});

		//Car
		DSLcar.init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(CarUniformBufferObject), 1},
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
		Penv.init(this, &VDenv, "shaders/EnvVert.spv", "shaders/EnvFrag.spv", {&DSLGlobal, &DSLenv}); 
		Pcar.init(this, &VDcar, "shaders/CarVert.spv", "shaders/CarFrag.spv", {&DSLGlobal, &DSLcar}); 

		//----------------Models----------------
		MSkyBox.init(this, &VDSkyBox, "models/SkyBoxCube.obj", OBJ);
		Mcar.init(this, &VDcar, "models/car.mgcg", MGCG);
		Mfloor.init(this, &VDenv, "models/road/straight.mgcg", MGCG);


		//----------------Textures----------------
		TSkyBox.init(this, "textures/starmap_g4k.jpg");
		Tcar.init(this, "textures/Textures_City.png"); 
		TStars.init(this, "textures/constellation_figures.png");
		Tstraight.init(this, "textures/Textures_City.png");
		
		// Descriptor pool sizes
		DPSZs.uniformBlocksInPool = 4;												//# of uniform buffers  (Global, SkyBox, Uniform, Car)
		DPSZs.texturesInPool = 4;	//forse sono solo 3 + 1		//# of textures (SkyBox, Stars, Environment, Car)
		DPSZs.setsInPool = 4;  														//# of DS (Global, SkyBox, Environment, Car)
		
		std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
		std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
		std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		//Descriptor Set initialization
		DSSkyBox.init(this, &DSLSkyBox, {&TSkyBox, &TStars});
		DSGlobal.init(this, &DSLGlobal, {});
		DSenv.init(this, &DSLenv, {&Tstraight}); 
		DScar.init(this, &DSLcar, {&Tcar});  
		
		//Pipeline Creation
		PSkyBox.create();
		Penv.create();
		Pcar.create();
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		//Pipelines Cleanup
		PSkyBox.cleanup();
		Penv.cleanup();
		Pcar.cleanup();

		//Descriptor Set Cleanup
		DSGlobal.cleanup();
		DSSkyBox.cleanup();
		DSenv.cleanup(); 
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
		Tstraight.cleanup();
		Tcar.cleanup(); 

		//Models Cleanup
		MSkyBox.cleanup();
		Mfloor.cleanup(); 
		Mcar.cleanup(); 

		//Descriptor Set Layouts Cleanup
		DSLGlobal.cleanup();
		DSLSkyBox.cleanup();
		DSLenv.cleanup();
		DSLcar.cleanup();
		
		//Pipelines destruction
		PSkyBox.destroy();
		Penv.destroy(); 
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

		//Draw Floor
		Penv.bind(commandBuffer); 
		Mfloor.bind(commandBuffer); 
		DSGlobal.bind(commandBuffer, Penv, 0, currentImage); 
		DSenv.bind(commandBuffer, Penv, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mfloor.indices.size()), STRAIGHT_ROAD_DIM, 0, 0, 0);
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		// Parameters for the SixAxis
		float deltaT;					// Time between frames
		glm::vec3 m = glm::vec3(0.0f);  // Movement
		glm::vec3 r = glm::vec3(0.0f);  // Rotation
		bool fire = false;				// Button pressed
		getSixAxis(deltaT, m, r, fire); 
	
		// Parameters for the Camera
		constexpr float ROT_SPEED = glm::radians(120.0f);
		constexpr float MOVE_SPEED = 2.0f;

		static float alpha = M_PI;					// yaw
		static float beta = glm::radians(5.0f);     // pitch
		static float camDist = 10.0f;				// distance from the target

		//Matrices setup 
		glm::mat4 pMat = glm::perspective(FOVy, Ar, nearPlane, farPlane);	//Projection Matrix
		pMat[1][1] *= -1;													//Flip Y
		glm::mat4 vpMat;													//View Projection Matrix


		/************************************* MOTION OF THE CAR *************************************/
		
		steeringAng += -m.x * carSteeringSpeed * deltaT;
		/*steeringAng = (steeringAng < glm::radians(-35.0f) ? glm::radians(-35.0f) :
			(steeringAng > glm::radians(35.0f) ? glm::radians(35.0f) : steeringAng));*/
		if (m.z != 0.0f) {
			carVelocity += carAcceleration * deltaT;
		}
		else {
			carVelocity = glm::max(0.0f, carVelocity - (carDecelerationEffect * deltaT));
		}
		if (m.z != 0.0f) {
			startingCarPos.z += carVelocity * deltaT * m.z * glm::cos(steeringAng);
			startingCarPos.x += carVelocity * deltaT * m.z * glm::sin(steeringAng);
		}
		else {
			startingCarPos.z += carVelocity * deltaT * glm::cos(steeringAng);
			startingCarPos.x += carVelocity * deltaT * glm::sin(steeringAng);
		}
	
		updatedCarPos.z = updatedCarPos.z * std::exp(-carDampingSpeed * deltaT) + startingCarPos.z * (1 - std::exp(-carDampingSpeed * deltaT));
		updatedCarPos.x = updatedCarPos.x * std::exp(-carDampingSpeed* deltaT) + startingCarPos.x * (1 - std::exp(-carDampingSpeed * deltaT));	
		/************************************* END MOTION OF THE CAR *************************************/
		
		
		
		//----------------Walk model procedure---------------- (In progress)
		// Walk model procedure
		glm::vec3 ux = glm::vec3(glm::rotate(glm::mat4(1), alpha, glm::vec3(0,1,0)) * glm::vec4(1,0,0,1));
		glm::vec3 uy = glm::vec3(0,1,0);
		glm::vec3 uz = glm::vec3(glm::rotate(glm::mat4(1), alpha, glm::vec3(0,1,0)) * glm::vec4(0,0,1,1));
		alpha -= ROT_SPEED * r.y * deltaT;	 // yaw
		beta -= ROT_SPEED * r.x * deltaT; // pitch
		//rho -= ROT_SPEED * r.z * deltaT;  // roll (not used)
		/*camPos -= ux * MOVE_SPEED * m.x * deltaT;
		camPos -= uy * MOVE_SPEED * m.y * deltaT; // Uncomment to enable vertical movement (can be used for camera distance)
		camPos -= uz * MOVE_SPEED * m.z * deltaT; */

		//glm::vec3 cameraOffset = glm::vec3(0.0f, 5.0f, -10.0f); // Adjust as needed
		//camPos = camTarget + cameraOffset;
		
		camPos = updatedCarPos + glm::vec3(0.0f, 2.0f, 5.0f);
		ViewMatrix = glm::lookAt(camPos, updatedCarPos, uy);
		vpMat = pMat * ViewMatrix; 
		//----------------------------------------------------

		//Update global uniforms				
		//Global
		GlobalUniformBufferObject g_ubo{};
		g_ubo.lightDir = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 2.5f);
		g_ubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		g_ubo.viewerPosition = camPos; 
		DSGlobal.map(currentImage, &g_ubo, 0);
		
		//Object Uniform Buffer creation
		//SkyBox
		skyBoxUniformBufferObject sb_ubo{};
		sb_ubo.mvpMat = pMat * glm::mat4(glm::mat3(ViewMatrix)); //Remove Translation part of ViewMatrix, take only Rotation part and applies Projection
		DSSkyBox.map(currentImage, &sb_ubo, 0);

		//Car
		CarUniformBufferObject car_ubo{}; 
		car_ubo.mMat = glm::translate(glm::mat4(1.0f), updatedCarPos) *
					   glm::rotate(glm::mat4(1.0f), glm::radians(180.0f) + steeringAng, glm::vec3(0, 1, 0)); 
		car_ubo.mvpMat = vpMat * car_ubo.mMat;
		car_ubo.nMat = glm::transpose(glm::inverse(car_ubo.mMat));
		DScar.map(currentImage, &car_ubo, 0);

		//Floor
		SraightRoadUniformBufferObject floor_ubo{};
		for (int i = 0; i < STRAIGHT_ROAD_DIM; i++) {
			floor_ubo.mMat[i] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -11.0f * i)) * 
								glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
			floor_ubo.mMat[i] = glm::translate(floor_ubo.mMat[i], glm::vec3(i * 5.0f, 0.0f, 0.0f));
			floor_ubo.mvpMat[i] = vpMat * floor_ubo.mMat[i];
			floor_ubo.nMat[i] = glm::transpose(glm::inverse(floor_ubo.mMat[i]));;
		}

		DSGlobal.map(currentImage, &floor_ubo, 1);
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
