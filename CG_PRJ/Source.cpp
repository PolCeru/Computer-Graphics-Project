#include "modules/Starter.hpp"

//Global
struct GlobalUniformBufferObject {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;
};

//Skybox
struct skyBoxUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
};

struct skyBoxVertex {
	glm::vec3 pos;
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

	//Application Parameters
	glm::vec3 CamPos = glm::vec3(0.0, 0.1, 5.0);
	glm::mat4 ViewMatrix = glm::mat4(1.0f);


	float Ar;
	
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
		//Descriptor Set Layout
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

		//Vertex Descriptor
		//Skybox
		VDSkyBox.init(this, {
				{0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
			}, {
				{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos), sizeof(glm::vec3), POSITION}
			});

		//Pipelines
		//SkyBox
		PSkyBox.init(this, &VDSkyBox, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", {&DSLSkyBox});
		PSkyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
 								    VK_CULL_MODE_BACK_BIT, false);

		//Models
		MSkyBox.init(this, &VDSkyBox, "models/SkyBoxCube.obj", OBJ);

		//Textures
		TSkyBox.init(this, "textures/starmap_g4k.jpg");
		TStars.init(this, "textures/constellation_figures.png");

		// Descriptor pool sizes
		DPSZs.uniformBlocksInPool = 2;
		DPSZs.texturesInPool = 3;
		DPSZs.setsInPool = 2;
		
		std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
		std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
		std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";

		ViewMatrix = glm::translate(glm::mat4(1), -CamPos);
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		//Pipeline Creation
		PSkyBox.create();


		//Descriptor Set initialization
		DSSkyBox.init(this, &DSLSkyBox, {&TSkyBox, &TStars});
		DSGlobal.init(this, &DSLGlobal, {});
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		//Pipelines Cleanup
		PSkyBox.cleanup();

		//Descriptor Set Cleanup
		DSSkyBox.cleanup();
	}

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two different
	// methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup() {	
		//Models Cleanup
		MSkyBox.cleanup();

		//Textures Cleanup
		TSkyBox.cleanup();
		TStars.cleanup();

		//Descriptor Set Layouts Cleanup
		DSLSkyBox.cleanup();
		DSLGlobal.cleanup();
		
		//Pipelines destruction
		PSkyBox.destroy();
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
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f);
		glm::vec3 r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);

		static float cTime = 0.0;
		const float turnTime = 72.0f;
		const float angTurnTimeFact = 2.0f * M_PI / turnTime;

		const float ROT_SPEED = glm::radians(120.0f);
		const float MOVE_SPEED = 2.0f;

		// The Fly model update proc.
		ViewMatrix = glm::rotate(glm::mat4(1), ROT_SPEED * r.x * deltaT,
								 glm::vec3(1, 0, 0)) * ViewMatrix;
		ViewMatrix = glm::rotate(glm::mat4(1), ROT_SPEED * r.y * deltaT,
								 glm::vec3(0, 1, 0)) * ViewMatrix;
		ViewMatrix = glm::rotate(glm::mat4(1), -ROT_SPEED * r.z * deltaT,
								 glm::vec3(0, 0, 1)) * ViewMatrix;
		ViewMatrix = glm::translate(glm::mat4(1), -glm::vec3(
								   MOVE_SPEED * m.x * deltaT, MOVE_SPEED * m.y * deltaT, MOVE_SPEED * m.z * deltaT))
													   * ViewMatrix;
		// updates global uniforms
		// Global
		GlobalUniformBufferObject gubo{};
		gubo.lightDir = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));
		gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gubo.eyePos = glm::vec3(glm::inverse(ViewMatrix) * glm::vec4(0, 0, 0, 1));
		DSGlobal.map(currentImage, &gubo, 0);

		//Matrixes
		glm::mat4 M = glm::perspective(glm::radians(45.0f), Ar, 0.1f, 160.0f);
		M[1][1] *= -1;
		glm::mat4 Mv = ViewMatrix;					

		//Object Uniform Buffer creation
		//SkyBox
		skyBoxUniformBufferObject sbubo{};
		sbubo.mvpMat = M * glm::mat4(glm::mat3(Mv));
		DSSkyBox.map(currentImage, &sbubo, 0);
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
