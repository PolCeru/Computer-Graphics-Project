#include "modules/Starter.hpp"

//Global
struct GlobalUniformBufferObject {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;
};

struct UniformBufferObject {
	alignas(16) glm::mat4 mvpMat; //World View Projection Matrix
	alignas(16) glm::mat4 mMat;   //Model/World Matrix
	alignas(16) glm::mat4 nMat;   //Normal Matrix
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
	Model Mcar, Mfloor; 
	Texture Tenv; 
	DescriptorSet DSenv, DScar; 

	//Application Parameters
	glm::vec3 camPos = glm::vec3(0.0, 2.0, 15.0); //Camera Position (-l/+r, -d/+u, b/f)
	glm::mat4 ViewMatrix = glm::translate(glm::mat4(1), -camPos); //View Matrix setup
	glm::vec3 camTarget = glm::vec3(0.0, 0.0, 0.0); //Car Position
	const glm::vec3 CamTargetDelta = glm::vec3(0,2,0);

	float Ar;
	float FOVy = glm::radians(60.0f);
	float nearPlane = 0.1f;
	float farPlane = 500.0f;

	/******* CAR PARAMETERS *******/
	glm::vec3 startingCarPos = glm::vec3(0.0f);
	glm::vec3 updatedCarPos = glm::vec3(0.0f);
	float steeringAng = 0.0f;
	float carVelocity = 0.0f;
	float carAcceleration = 1.0f;  // realistically it should depend on the acceleration of the car
	float carDeceleration = 0.0f; // Not implemented the logic yer, but it should depend on the car brake
	float carDecelerationEffect = 10.0f; //realistically it should depend on the surface attrition 
	float carSteeringSpeed = glm::radians(30.0f);
	float carDampingSpeed = 1.5f; 
	bool goStraightOnZ = false; 
	bool goStraightOnX = false; 


	
	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "A10 - Adding an object";
    	windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};
		
		Ar = (float)windowWidth / (float)windowHeight;
	}
	
	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		std::cout << "Window resized to: " << w << " x " << h << "\n";
		Ar = (float)w / (float)h;
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

		//Environment
		DSLenv.init(this, {
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
				{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv), sizeof(glm::vec2), UV}
			});

		//----------------Pipelines----------------
		//SkyBox
		PSkyBox.init(this, &VDSkyBox, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", {&DSLSkyBox});
		PSkyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false); 
		// Environment	
		Penv.init(this, &VDenv, "shaders/EnvVert.spv", "shaders/EnvFrag.spv", {&DSLenv}); 

		//----------------Models----------------
		MSkyBox.init(this, &VDSkyBox, "models/SkyBoxCube.obj", OBJ);
		Mcar.init(this, &VDenv, "models/car.mgcg", MGCG);
		Mfloor.init(this, &VDenv, "models/LargePlane.obj", OBJ);

		//----------------Textures----------------
		TSkyBox.init(this, "textures/starmap_g4k.jpg");
		TStars.init(this, "textures/constellation_figures.png");
		Tenv.init(this, "textures/Textures_City.png");
		
		// Descriptor pool sizes
		DPSZs.uniformBlocksInPool = 4;	//# of uniform buffers  (Global, SkyBox, Uniform, Car)
		DPSZs.texturesInPool = 4;		//# of textures			(SkyBox, Stars, Environment, Car)
		DPSZs.setsInPool = 4;  			//# of DS				(Global, SkyBox, Environment, Car)
		
		std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
		std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
		std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		//Descriptor Set initialization
		DSSkyBox.init(this, &DSLSkyBox, {&TSkyBox, &TStars});
		DSGlobal.init(this, &DSLGlobal, {});
		DScar.init(this, &DSLenv, {&Tenv}); 
		DSenv.init(this, &DSLenv, {&Tenv}); 
		
		//Pipeline Creation
		PSkyBox.create();
		Penv.create();
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		//Pipelines Cleanup
		PSkyBox.cleanup();
		Penv.cleanup();

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
		Tenv.cleanup();

		//Models Cleanup
		MSkyBox.cleanup();
		Mcar.cleanup(); 
		Mfloor.cleanup();

		//Descriptor Set Layouts Cleanup
		DSLGlobal.cleanup();
		DSLSkyBox.cleanup();
		DSLenv.cleanup();
		
		//Pipelines destruction
		PSkyBox.destroy();
		Penv.destroy(); 
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
		Penv.bind(commandBuffer); 
		Mcar.bind(commandBuffer); 
		DScar.bind(commandBuffer, Penv, 0, currentImage); 
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mcar.indices.size()), 1, 0, 0, 0);

		//Draw Floor
		Penv.bind(commandBuffer); 
		Mfloor.bind(commandBuffer); 
		DSenv.bind(commandBuffer, Penv, 0, currentImage); 
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mfloor.indices.size()), 1, 0, 0, 0);
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

		static float alpha = M_PI;				// yaw
		static float beta = glm::radians(5.0f);     // pitch
		static float camDist = 10.0f;				// distance from the target

		//Matrices setup 
		glm::mat4 pMat = glm::perspective(FOVy, Ar, nearPlane, farPlane);	//Projection Matrix
		pMat[1][1] *= -1;													//Flip Y
		glm::mat4 vpMat;													//View Projection Matrix


		/***************************************** MOTION OF THE CAR ****************************************************/
		
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

	
		/*******************************************END MOTION OF THE CAR ****************************************/
		
		
		
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
		//glm::mat4 carModelMatrix = glm::translate(glm::mat4(1.0f), camTarget); // Example: translate to car's position
 
		//----------------------------------------------------

		/*----------------fly model procedure---------------- (not used)
		ViewMatrix = glm::rotate(glm::mat4(1), ROT_SPEED * r.x * deltaT, glm::vec3(1, 0, 0)) * ViewMatrix;
		ViewMatrix = glm::rotate(glm::mat4(1), ROT_SPEED * r.y * deltaT, glm::vec3(0, 1, 0)) * ViewMatrix;
		ViewMatrix = glm::rotate(glm::mat4(1), -ROT_SPEED * r.z * deltaT, glm::vec3(0, 0, 1)) * ViewMatrix;
		ViewMatrix = glm::translate(glm::mat4(1), -glm::vec3(MOVE_SPEED * m.x * deltaT, MOVE_SPEED * m.y * deltaT, MOVE_SPEED * m.z * deltaT)) * ViewMatrix;
		vpMat = pMat * ViewMatrix;
		*/

		//Update global uniforms				
		//Global
		GlobalUniformBufferObject g_ubo{};
		g_ubo.lightDir = glm::vec3(1.0f);
		g_ubo.lightColor = glm::vec4(1.0f);
		g_ubo.eyePos = glm::vec3(1.0f);
		DSGlobal.map(currentImage, &g_ubo, 0);
		
		//Object Uniform Buffer creation
		//SkyBox
		skyBoxUniformBufferObject sb_ubo{};
		sb_ubo.mvpMat = pMat * glm::mat4(glm::mat3(ViewMatrix)); //Remove Translation part of ViewMatrix, take only Rotation part and applies Projection
		DSSkyBox.map(currentImage, &sb_ubo, 0);

		//Car
		UniformBufferObject car_ubo{}; 
		car_ubo.mMat = glm::translate(glm::mat4(1.0f), updatedCarPos) *
						glm::rotate(glm::mat4(1.0f), glm::radians(180.0f) + steeringAng, glm::vec3(0, 1, 0)); 
		car_ubo.mvpMat = vpMat * car_ubo.mMat;
		car_ubo.nMat = glm::transpose(glm::inverse(car_ubo.mMat));
		DScar.map(currentImage, &car_ubo, 0);

		//Floor
		UniformBufferObject floor_ubo{}; 
		floor_ubo.mMat = glm::mat4(1.0f); //glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, 0.1));
		floor_ubo.mvpMat = vpMat * floor_ubo.mMat;
		floor_ubo.nMat = glm::transpose(glm::inverse(floor_ubo.mMat));;
		DSenv.map(currentImage, &floor_ubo, 0);
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
