#include "modules/Starter.hpp"
#include <filesystem>
#include <map>
#include <string>
#include <random>

#define MAP_SIZE 15
#define DIRECTIONS 4
#define SCALING_FACTOR 16.0f

//Global
struct GlobalUniformBufferObject {
	//Direct Light
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 viewerPosition;
};

//Car
struct UniformBufferObject {
	alignas(16) glm::mat4 mvpMat; //World View Projection Matrix
	alignas(16) glm::mat4 mMat;   //Model/World Matrix
	alignas(16) glm::mat4 nMat;   //Normal Matrix
};

struct CarLightsUniformBufferObject {
    // Headlights
    alignas(16) glm::vec3 headlightPosition[2];  //left and right
    alignas(16) glm::vec3 headlightDirection[2];
    alignas(16) glm::vec4 headlightColor[2];

    //Rear lights
    alignas(16) glm::vec3 rearLightPosition[2];  //left and right
    alignas(16) glm::vec3 rearLightDirection[2]; 
    alignas(16) glm::vec4 rearLightColor[2];     
};

//Road
struct RoadUniformBufferObject {
	alignas(16) glm::mat4 mvpMat[MAP_SIZE * MAP_SIZE];
	alignas(16) glm::mat4 mMat[MAP_SIZE * MAP_SIZE];
	alignas(16) glm::mat4 nMat[MAP_SIZE * MAP_SIZE];
};

struct RoadLightsUniformBufferObject {
	alignas(16) glm::vec4 spotLight_lightPosition[MAP_SIZE * MAP_SIZE][3];
	alignas(16) glm::vec4 spotLight_spotDirection[MAP_SIZE * MAP_SIZE][3];
	alignas(16) glm::vec4 lightColorSpot;
};

struct RoadPosition {
	glm::vec3 pos;
	int type;
	float rotation;
};

//Road Types
enum RoadType {
	STRAIGHT = 0,
	LEFT = 1,
	RIGHT = 2,
	NONE = 3
};

//Environment
struct EnvironmentUniformBufferObject {
	alignas(16) glm::mat4 mvpMat[MAP_SIZE * MAP_SIZE];
	alignas(16) glm::mat4 mMat[MAP_SIZE * MAP_SIZE];
	alignas(16) glm::mat4 nMat[MAP_SIZE * MAP_SIZE];
};

struct Vertex {
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 normal;
};

//Skybox
struct skyBoxUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
};

struct skyBoxVertex {
	glm::vec3 pos;
};

// MAIN
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
	Texture TSkyBox, TStars, Tclouds, Tsunrise, Tday, Tsunset;
	DescriptorSet DSSkyBox;

	//Road
	DescriptorSetLayout DSLroad;
	VertexDescriptor VDroad;
	Pipeline Proad;
	Model MstraightRoad;
	Model MturnLeft;
	Model MturnRight;
	Model Mtile;
	DescriptorSet DSstraightRoad;
	DescriptorSet DSturnLeft;
	DescriptorSet DSturnRight;
	DescriptorSet DStile;
	std::vector<std::vector<RoadPosition>> mapLoaded;
	std::vector<std::vector<std::pair<int, int>>> mapIndexes; // 0: STRAIGHT, 1: LEFT, 2: RIGHT

	//Car
	DescriptorSetLayout DSLcar;
	VertexDescriptor VDcar;
	Pipeline Pcar;
	Model Mcar;
	DescriptorSet DScar;

	// Environment
	DescriptorSetLayout DSLenvironment;
	VertexDescriptor VDenv;
	Pipeline Penv;
	std::vector<Model> Menv;
	Texture Tenv;
	std::vector<DescriptorSet> DSenvironment;
	std::map<int, std::string> envFileNames;
	const std::string envModelsPath = "models/environment";
	std::vector<std::vector<std::pair <int, int>>> envIndexesPerModel;


	/******* APP PARAMETERS *******/
	float ar;
	float FOVy = glm::radians(75.0f);
	float nearPlane = 0.1f;
	float farPlane = 500.0f;
	const int MAP_CENTER = MAP_SIZE / 2;
	const float baseObjectRotation = 90.0f;

	/******* CAMERA PARAMETERS *******/
	float alpha = M_PI;					// yaw
	float beta = glm::radians(5.0f);    // pitch
	//static float rho = 0.0f;			// roll
	float camDist = 7.0f;				// distance from the target
	float camHeight = 2.0f;				// height from the target
	const float lambdaCam = 10.0f;      // damping factor for the camera

	glm::vec3 camPos = glm::vec3(0.0, camHeight, camDist);				//Camera Position (-l/+r, -d/+u, b/f)
	glm::mat4 viewMatrix = glm::translate(glm::mat4(1), -camPos);		//View Matrix setup
	glm::vec3 upVector = glm::vec3(0, 1, 0);							//Up Vector

	/******* CAR PARAMETERS *******/
	glm::vec3 startingCarPos = glm::vec3(-28.0f, 0.0f, 0.0f);
	glm::vec3 updatedCarPos = glm::vec3(-28.0f, 0.0f, 0.0f);
	const float ROT_SPEED = glm::radians(120.0f);
	const float MOVE_SPEED = 2.0f;
	const float carAcceleration = 8.0f;						// [m/s^2]
	const float carDeceleration = 4.0f;
	const float friction = 0.7f;
	const float carSteeringSpeed = glm::radians(30.0f);
	const float carDampingSpeed = 5.0f;
	const float braking = 6.0f;
	float steeringAng = 0.0f;
	float carVelocity = 0.0f;

	/************ DAY PHASES PARAMETERS *****************/
	float turningTime = 0.0f; 
	float sun_cycle_duration = 60.0f;
	float daily_phase_duration = sun_cycle_duration / 3.0f; 
	float rad_per_sec = M_PI / sun_cycle_duration;
	float timeScene = 0.0f; 
	glm::vec4 sunriseColor = glm::vec4(1.0f, 0.39f, 0.28f, 1.0f);
	glm::vec4 dayColor = glm::vec4(0.9f, 0.8f, 0.3f, 1.0f);
	glm::vec4 sunsetColor = glm::vec4(1.0f, 0.55f, 0.2f, 1.0f);
	glm::vec4 nightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Scene 
	int scene = 0; 

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
				{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(RoadUniformBufferObject), 1},
				{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(CarLightsUniformBufferObject), 1},
				{3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(RoadLightsUniformBufferObject), 1}
			});

		//Car
		DSLcar.init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObject), 1},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
				{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(CarLightsUniformBufferObject), 1}
			});

		DSLenvironment.init(this, {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(EnvironmentUniformBufferObject), 1},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
			});

		//----------------Vertex Descriptor----------------
		//Skybox
		VDSkyBox.init(this, {
				{0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
			}, {
				{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos), sizeof(glm::vec3), POSITION}
			});

		//Road
		VDroad.init(this, {
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

		//Environment
		VDenv.init(this, {
			{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
		}, {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
			{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv), sizeof(glm::vec2), UV},
			{0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal), sizeof(glm::vec3), NORMAL},
		});

		//----------------Pipelines----------------
		PSkyBox.init(this, &VDSkyBox, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", { &DSLSkyBox });
		PSkyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false);
		Proad.init(this, &VDroad, "shaders/RoadVert.spv", "shaders/RoadFrag.spv", { &DSLGlobal, &DSLroad });
		Proad.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Pcar.init(this, &VDcar, "shaders/CarVert.spv", "shaders/CarFrag.spv", { &DSLGlobal, &DSLcar });
		Penv.init(this, &VDenv, "shaders/EnvVert.spv", "shaders/EnvFrag.spv", { &DSLGlobal, &DSLenvironment });
		Penv.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);

		//----------------Models----------------
		MSkyBox.init(this, &VDSkyBox, "models/SkyBoxCube.obj", OBJ);
		Mcar.init(this, &VDcar, "models/car.mgcg", MGCG);
		MstraightRoad.init(this, &VDroad, "models/road/straight.mgcg", MGCG);
		MturnLeft.init(this, &VDroad, "models/road/turn.mgcg", MGCG);
		MturnRight.init(this, &VDroad, "models/road/turn.mgcg", MGCG);
		Mtile.init(this, &VDroad, "models/road/green_tile.mgcg", MGCG);

		//----------------Map Grid Initialization----------------
		auto mapMatrix = LoadMapFile();
		LoadMap(mapMatrix);
		
		//Environment models
		readModels(envModelsPath);
		Menv.resize(envFileNames.size());
		for (const auto& [key, value] : envFileNames) {
			Menv[key].init(this, &VDenv, value, MGCG);
		}

		//----------------Textures----------------
		TSkyBox.init(this, "textures/starmap_g4k.jpg");
		Tenv.init(this, "textures/Textures_City.png");
		TStars.init(this, "textures/constellation_figures.png");
		Tclouds.init(this, "textures/Clouds.jpg");
		Tsunrise.init(this, "textures/SkySunrise.png"); 
		Tday.init(this, "textures/SkyDay.png"); 
		Tsunset.init(this, "textures/SkySunset.png");


		DPSZs.uniformBlocksInPool = 1 + 1 + 12 + 2 + Menv.size();  // summation of (#ubo * #DS) for each DSL
		DPSZs.texturesInPool = 2 + 4 + 1 + Menv.size();			   // summation of (#texure * #DS) for each DSL
		DPSZs.setsInPool = 7 + Menv.size();						  // summation of #DS for each DSL

		std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
		std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
		std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";

		std::random_device rd;										//Obtain a random number from hardware
		std::mt19937 gen(rd());										//Seed the generator
		std::uniform_int_distribution<> distr(0, Menv.size()-1);	//Define the range
		
		//Random distribution of environment models on the map
		envIndexesPerModel.resize(Menv.size());
		for(int i = 0; i < mapIndexes[NONE].size(); i++) {
	        int modelNumber = distr(gen);
			std::pair <int, int> index = mapIndexes[NONE][i];
			envIndexesPerModel[modelNumber].push_back(index);
		}
	}

	void readModels(std::string path){
		std::vector<std::string> directories;
		int id = 0;

		try {
			for (const auto& entry : std::filesystem::directory_iterator(path)) {
				if (entry.is_directory()) {
					directories.push_back(path+"/"+entry.path().filename().generic_string());
				}
			}
		} catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "Filesystem error: " << e.what() << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "General error: " << e.what() << std::endl;
		}

		for (const auto& dir : directories) {
			try {
				for (const auto& entry : std::filesystem::directory_iterator(dir)) {
					if (entry.is_regular_file()) {
						envFileNames[id++] = entry.path().generic_string();
					}
				}
			} catch (const std::filesystem::filesystem_error& e) {
				std::cerr << "Filesystem error: " << e.what() << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "General error: " << e.what() << std::endl;
			}
		}
	}

	// Loads the JSON
	nlohmann::json LoadMapFile(){
		nlohmann::json json;

		std::ifstream infile("config/map1.json");
		if (!infile.is_open()) {
			std::cerr << "Error opening file!" << std::endl;
			exit(1);
		}

		// Parse the JSON content
		infile >> json;
		infile.close();

		return json["map"];
	}

	// Initialize the mapLoaded and mapIndexes
	void LoadMap(nlohmann::json& mapMatrix)
	{
		mapIndexes.resize(DIRECTIONS);
		mapLoaded.resize(MAP_SIZE, std::vector<RoadPosition>(MAP_SIZE));
		for (int i = 0; i < mapLoaded.size(); i++) {
			for (int j = 0; j < MAP_SIZE; j++) {
				int type = mapMatrix[i][j][0];
				std::pair <int, int> index = std::make_pair(i, j);

				float x = SCALING_FACTOR * (j - MAP_CENTER);
				float z = SCALING_FACTOR * (i - MAP_CENTER);

				mapLoaded[i][j].pos = glm::vec3(x, 0.0f, z);
				mapLoaded[i][j].type = type;
				mapLoaded[i][j].rotation = mapMatrix[i][j][1];

				switch (type) {
				case STRAIGHT:
					mapIndexes[STRAIGHT].push_back(index);
					break;

				case LEFT:
					mapIndexes[LEFT].push_back(index);
					break;

				case RIGHT:
					mapIndexes[RIGHT].push_back(index);
					break;

				default:
					mapIndexes[NONE].push_back(index);
					break;
				}
			}
		}
	}

	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		//Descriptor Set initialization

		switch (scene) {
			case 0 : 
				DSSkyBox.init(this, &DSLSkyBox, { &Tclouds, &Tsunrise });
				break; 
			case 1:
				DSSkyBox.init(this, &DSLSkyBox, { &Tclouds, &Tday });
				break;
			case 2:
				DSSkyBox.init(this, &DSLSkyBox, { &Tclouds, &Tsunset });
				break; 
			default:
				DSSkyBox.init(this, &DSLSkyBox, { &TSkyBox, &TStars });
				break;
		}

		DSGlobal.init(this, &DSLGlobal, { });
		DSstraightRoad.init(this, &DSLroad, { &Tenv });
		DSturnLeft.init(this, &DSLroad, { &Tenv });
		DSturnRight.init(this, &DSLroad, { &Tenv });
		DStile.init(this, &DSLroad, { &Tenv });
		DScar.init(this, &DSLcar, { &Tenv });  
		DSenvironment.resize(Menv.size());
		for (int i = 0; i < DSenvironment.size(); i++) {
			DSenvironment[i].init(this, &DSLenvironment, { &Tenv });
		}
		
		//Pipeline Creation
		PSkyBox.create();
		Proad.create();
		Pcar.create();
		Penv.create();
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		//Pipelines Cleanup
		PSkyBox.cleanup();
		Proad.cleanup();
		Pcar.cleanup();
		Penv.cleanup();

		//Descriptor Set Cleanup
		DSGlobal.cleanup();
		DSSkyBox.cleanup();
		DSstraightRoad.cleanup();
		DSturnLeft.cleanup();
		DSturnRight.cleanup();
		DStile.cleanup();
		DScar.cleanup();
		for (int i = 0; i < DSenvironment.size(); i++) {
			DSenvironment[i].cleanup();
		}
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
		Tsunrise.cleanup(); 
		Tday.cleanup(); 
		Tsunset.cleanup(); 
		Tclouds.cleanup(); 
		


		//Models Cleanup
		MSkyBox.cleanup();
		MstraightRoad.cleanup();
		MturnLeft.cleanup();
		MturnRight.cleanup();
		Mtile.cleanup();
		Mcar.cleanup();
		for (int i = 0; i < Menv.size(); i++) {
			Menv[i].cleanup();
		}

		//Descriptor Set Layouts Cleanup
		DSLGlobal.cleanup();
		DSLSkyBox.cleanup();
		DSLroad.cleanup();
		DSLcar.cleanup();
		DSLenvironment.cleanup();

		//Pipelines destruction
		PSkyBox.destroy();
		Proad.destroy();
		Pcar.destroy();
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
		Pcar.bind(commandBuffer);
		Mcar.bind(commandBuffer);
		DSGlobal.bind(commandBuffer, Pcar, 0, currentImage);
		DScar.bind(commandBuffer, Pcar, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mcar.indices.size()), 1, 0, 0, 0);

		//Draw Road pieces
		Proad.bind(commandBuffer);

		MstraightRoad.bind(commandBuffer);
		DSstraightRoad.bind(commandBuffer, Proad, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MstraightRoad.indices.size()), static_cast<uint32_t>(mapIndexes[STRAIGHT].size()), 0, 0, 0);

		MturnLeft.bind(commandBuffer);
		DSturnLeft.bind(commandBuffer, Proad, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MturnLeft.indices.size()), static_cast<uint32_t>(mapIndexes[LEFT].size()), 0, 0, 0);

		MturnRight.bind(commandBuffer);
		DSturnRight.bind(commandBuffer, Proad, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(MturnRight.indices.size()), static_cast<uint32_t>(mapIndexes[RIGHT].size()), 0, 0, 0);

		Mtile.bind(commandBuffer);
		DStile.bind(commandBuffer, Proad, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mtile.indices.size()), static_cast<uint32_t>(mapIndexes[NONE].size()), 0, 0, 0);

		//Draw Environment
		//extract number, count uniqueness (map) and i = id, menv.size() = uniqueness
		Penv.bind(commandBuffer);
		for (int i = 0; i < Menv.size(); i++) {
			Menv[i].bind(commandBuffer);
			DSGlobal.bind(commandBuffer, Penv, 0, currentImage);
			DSenvironment[i].bind(commandBuffer, Penv, 1, currentImage);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Menv[i].indices.size()), static_cast<uint32_t>(envIndexesPerModel[i].size()), 0, 0, 0);
		}
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
			carVelocity -= friction * 9.81 * deltaT;
			carVelocity = glm::max(carVelocity, 0.0f);
		}

		if (carVelocity != 0.0f)
			steeringAng += -m.x * carSteeringSpeed * deltaT;

		/*steeringAng = (steeringAng < glm::radians(-35.0f) ? glm::radians(-35.0f) :
			(steeringAng > glm::radians(35.0f) ? glm::radians(35.0f) : steeringAng));*/

		startingCarPos.z -= carVelocity * deltaT * glm::cos(steeringAng);
		startingCarPos.x -= carVelocity * deltaT * glm::sin(steeringAng);
		updatedCarPos.z = updatedCarPos.z * std::exp(-carDampingSpeed * deltaT) + startingCarPos.z * (1 - std::exp(-carDampingSpeed * deltaT));
		updatedCarPos.x = updatedCarPos.x * std::exp(-carDampingSpeed * deltaT) + startingCarPos.x * (1 - std::exp(-carDampingSpeed * deltaT));

		updatedCarPos.x = (updatedCarPos.x < -SCALING_FACTOR * MAP_CENTER ? (- SCALING_FACTOR * MAP_CENTER)+0.1f : (updatedCarPos.x > SCALING_FACTOR * MAP_CENTER ? (SCALING_FACTOR * MAP_CENTER)-0.1f  : updatedCarPos.x)); //boundaries
		updatedCarPos.z = (updatedCarPos.z < -SCALING_FACTOR * MAP_CENTER ? (- SCALING_FACTOR * MAP_CENTER)+0.1f : (updatedCarPos.z > SCALING_FACTOR * MAP_CENTER ? (SCALING_FACTOR * MAP_CENTER)-0.1f  : updatedCarPos.z)); //boundaries

		/************************************* Walk model procedure *************************************/
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

		camPos = updatedCarPos + glm::vec3(-glm::rotate(glm::mat4(1), alpha + steeringAng, glm::vec3(0, 1, 0)) *
			glm::rotate(glm::mat4(1), beta, glm::vec3(1, 0, 0)) *
			glm::vec4(0, -camHeight, camDist, 1));

		dampedCamPos = camPos * (1 - exp(-lambdaCam * deltaT)) + dampedCamPos * exp(-lambdaCam * deltaT);

		viewMatrix = glm::lookAt(dampedCamPos, updatedCarPos, upVector);
		vpMat = pMat * viewMatrix;
		/************************************************************************************************/
		
		turningTime += deltaT;

		turningTime = (turningTime >= 2.0 * sun_cycle_duration) ? 0.0f : turningTime;


		if (turningTime > daily_phase_duration && scene == 0) {
			scene = 1; 
			RebuildPipeline();
		}

		if (turningTime > 2.0f * daily_phase_duration && scene == 1) {
			scene = 2; 
			RebuildPipeline(); 
		}

		if (turningTime > sun_cycle_duration && scene == 2) {
			scene = 3; 
			RebuildPipeline(); 
		}

		if (turningTime <= daily_phase_duration && scene == 3) {
			scene = 0; 
			RebuildPipeline(); 
		}
		
		

		//Update global uniforms				
		//Global

		GlobalUniformBufferObject g_ubo{};

		if (scene != 3) {
			g_ubo.lightDir = glm::vec3(0.0f, sin(glm::radians(180.0f) - rad_per_sec * turningTime), cos(glm::radians(180.0f) - rad_per_sec * turningTime));
		}
		else {
			g_ubo.lightDir = glm::vec3(0.0f, sin(glm::radians(180.0f) - rad_per_sec * (turningTime - sun_cycle_duration)), cos(glm::radians(180.0f) - rad_per_sec * (turningTime - sun_cycle_duration)));
		}

		timeScene = turningTime - scene * daily_phase_duration;

		switch (scene) {
			case 0: //from sunrise to day
				g_ubo.lightColor = glm::vec4(sunriseColor.x + ((dayColor.x - sunriseColor.x) / daily_phase_duration) * timeScene, sunriseColor.y + ((dayColor.y - sunriseColor.y) / daily_phase_duration) * timeScene, sunriseColor.z + ((dayColor.z - sunriseColor.z) / daily_phase_duration) * timeScene, 1.0f);
				break; 
			case 1: // from day to sunset
				g_ubo.lightColor = glm::vec4(dayColor.x + ((sunsetColor.x - dayColor.y) / daily_phase_duration) * timeScene, dayColor.y + ((sunsetColor.y - dayColor.y) / daily_phase_duration) * timeScene, dayColor.z + ((sunsetColor.z - dayColor.z) / daily_phase_duration) * timeScene, 1.0f);
				break;
			case 2: //from sunset to night
				g_ubo.lightColor = glm::vec4(sunsetColor.x - (sunsetColor.x / daily_phase_duration) * timeScene, sunsetColor.y - (sunsetColor.y / daily_phase_duration) * timeScene, sunsetColor.z - (sunsetColor.z / daily_phase_duration) * timeScene, 1.0f);
				break; 
			default: // night
				g_ubo.lightColor = nightColor; 
				break; 
		}
		g_ubo.viewerPosition = glm::vec3(glm::inverse(viewMatrix) * glm::vec4(0, 0, 0, 1)); // would dampedCam make sense?
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

		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), steeringAng, glm::vec3(0.0f, 1.0f, 0.0f));
		CarLightsUniformBufferObject carLights_ubo{};
		for(int i = 0; i < 2; i++){
			glm::vec3 lightsOffset = glm::vec3((i == 0) ? -0.5f : 0.5f, 0.6f, -1.5f);
			carLights_ubo.headlightPosition[i] = updatedCarPos + glm::vec3(rotationMatrix * glm::vec4(lightsOffset, 1.0f));
			carLights_ubo.headlightDirection[i] = glm::vec3(rotationMatrix * glm::vec4(0.0f, -0.5f, -1.0f, 0.0f)); //pointing forward
			//carLights_ubo.headlightColor[i] = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f); //white
			carLights_ubo.headlightColor[i] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); //white

			lightsOffset = glm::vec3((i == 0) ? -0.55f : 0.55f, 0.6f, 1.9f);
			carLights_ubo.rearLightPosition[i] = updatedCarPos + glm::vec3(rotationMatrix * glm::vec4(lightsOffset, 1.0f));
			carLights_ubo.rearLightDirection[i] = glm::vec3(rotationMatrix * glm::vec4(0.0f, -0.5f, 1.0f, 0.0f)); //pointing backwards
			//carLights_ubo.rearLightColor[i] = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f); //red
			carLights_ubo.rearLightColor[i] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); //red

		}
		DScar.map(currentImage, &carLights_ubo, 2);

		//Road
		RoadUniformBufferObject straight_road_ubo{};
		RoadLightsUniformBufferObject lights_straight_road_ubo{}; 
		for (int i = 0; i < mapIndexes[STRAIGHT].size(); i++) {
			int n = mapIndexes[STRAIGHT][i].first;
			int m = mapIndexes[STRAIGHT][i].second;
			straight_road_ubo.mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
										glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation + baseObjectRotation), glm::vec3(0, 1, 0));
			straight_road_ubo.mvpMat[i] = vpMat * straight_road_ubo.mMat[i];
			straight_road_ubo.nMat[i] = glm::inverse(glm::transpose(straight_road_ubo.mMat[i]));
			
			lights_straight_road_ubo.spotLight_lightPosition[i][0] =
				glm::vec4(
					(glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
						glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0))) *
					glm::vec4(-4.9f, 5.0f, 0.0f, 1.0f)

				);
			lights_straight_road_ubo.spotLight_spotDirection[i][0] =
				glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0)) *  
				glm::vec4(0.15f, -1.0f, 0.0f, 1.0f);

			lights_straight_road_ubo.spotLight_lightPosition[i][1] =
				glm::vec4(
					(glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
						glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0))) *
					glm::vec4(4.9f, 5.0f, 8.0f, 1.0f)
				);

			lights_straight_road_ubo.spotLight_spotDirection[i][1] = 
				glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0)) *
				glm::vec4(-0.15f, -1.0f, 0.0f, 1.0f);
		
			bool conditionMet = false;
			int directions[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} }; // Up, Down, Left, Right

			for (int i = 0; i < 4; i++) {
				int newN = n + directions[i][0];
				int newM = m + directions[i][1];

				if (mapLoaded[newN][newM].type == 1 || mapLoaded[newN][newM].type == 2) {
					conditionMet = true;
					break;
				}
			}

			//if (!conditionMet) {
				lights_straight_road_ubo.spotLight_lightPosition[i][2] =
					glm::vec4(
						(glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
							glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0))) *
						glm::vec4(4.9f, 5.0f, -8.0f, 1.0f)
					);

				lights_straight_road_ubo.spotLight_spotDirection[i][2] =
					glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0)) *
					glm::vec4(-0.15f, -1.0f, 0.0f, 1.0f);
			//}

		}
		if (scene == 3) {
			lights_straight_road_ubo.lightColorSpot = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
		}
		else {
			lights_straight_road_ubo.lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		DSstraightRoad.map(currentImage, &straight_road_ubo, 1);
		DSstraightRoad.map(currentImage, &carLights_ubo, 2);
		DSstraightRoad.map(currentImage, &lights_straight_road_ubo, 3);

		RoadUniformBufferObject turn_right{};
		RoadLightsUniformBufferObject lights_turn_right_road_ubo{};
		for (int i = 0; i < mapIndexes[RIGHT].size(); i++) {
			int n = mapIndexes[RIGHT][i].first;
			int m = mapIndexes[RIGHT][i].second;
			turn_right.mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
								 glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation + baseObjectRotation), glm::vec3(0, 1, 0));
			turn_right.mvpMat[i] = vpMat * turn_right.mMat[i];
			turn_right.nMat[i] = glm::inverse(glm::transpose(turn_right.mMat[i]));		
			lights_turn_right_road_ubo.spotLight_lightPosition[i][0] =
				glm::vec4(
					(glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
						glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0))) *
					glm::vec4(-4.9f, 5.0f, -4.0f, 1.0f)

				);
			lights_turn_right_road_ubo.spotLight_spotDirection[i][0] =
				glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0)) *
				glm::vec4(0.15f, -1.0f, 0.0f, 1.0f);

			lights_turn_right_road_ubo.spotLight_lightPosition[i][1] =
				glm::vec4(
					(glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
						glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0))) *
					glm::vec4(4.9f, 5.0f, 6.0f, 1.0f)
				);

			lights_turn_right_road_ubo.spotLight_spotDirection[i][1] =
				glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0)) *
				glm::vec4(-0.15f, -1.0f, 0.0f, 1.0f);


			lights_turn_right_road_ubo.spotLight_lightPosition[i][2] =
				glm::vec4(
					(glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
						glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0))) *
					glm::vec4(2.0f, 5.0f, -4.0f, 1.0f)
				);

			lights_turn_right_road_ubo.spotLight_spotDirection[i][2] =
				glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0)) *
				glm::vec4(0.0f, -1.0f, 0.15f, 1.0f);
		}

		if (scene == 3) {
			lights_turn_right_road_ubo.lightColorSpot = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
		}
		else {
			lights_turn_right_road_ubo.lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		DSturnRight.map(currentImage, &turn_right, 1);
		DSturnRight.map(currentImage, &carLights_ubo, 2);
		DSturnRight.map(currentImage, &lights_turn_right_road_ubo, 3);


		RoadUniformBufferObject turn_left{};
		RoadLightsUniformBufferObject lights_turn_left_road_ubo{}; 
		for (int i = 0; i < mapIndexes[LEFT].size(); i++) {
			int n = mapIndexes[LEFT][i].first;
			int m = mapIndexes[LEFT][i].second;
			turn_left.mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
								glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation + baseObjectRotation), glm::vec3(0, 1, 0));
			turn_left.mvpMat[i] = vpMat * turn_left.mMat[i];
			turn_left.nMat[i] = glm::inverse(glm::transpose(turn_left.mMat[i]));
			lights_turn_left_road_ubo.spotLight_lightPosition[i][0] = 
				glm::vec4(
					(glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
						glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation - 90.0f), glm::vec3(0, 1, 0))) *
					glm::vec4(-5.0f, 5.0f, 6.0f, 1.0f)
				);
			lights_turn_left_road_ubo.spotLight_spotDirection[i][0] = 
				glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation - 90.0f), glm::vec3(0, 1, 0)) *
				glm::vec4(0.0f, -1.0f, -0.15f, 1.0f);
				
			lights_turn_left_road_ubo.spotLight_lightPosition[i][1] =
				glm::vec4(
					(glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
						glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation - 90.0f), glm::vec3(0, 1, 0))) *
					glm::vec4(5.0f, 5.0f, -6.0f, 1.0f)
				);
			lights_turn_left_road_ubo.spotLight_spotDirection[i][1] =
				glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation - 90.0f), glm::vec3(0, 1, 0)) *
				glm::vec4(0.0f, -1.0f , 0.15f, 1.0f);

			lights_turn_left_road_ubo.spotLight_lightPosition[i][2] =
				glm::vec4(
					(glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
						glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation - 90.0f), glm::vec3(0, 1, 0))) *
					glm::vec4(-5.0f, 5.0f, -2.0f, 1.0f)
				);
			lights_turn_left_road_ubo.spotLight_spotDirection[i][2] =
				glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation - 90.0f), glm::vec3(0, 1, 0)) *
				glm::vec4(0.15f, -1.0f, 0.0f, 1.0f);
		}

		if (scene == 3) {
			lights_turn_left_road_ubo.lightColorSpot = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
		}
		else{
			lights_turn_left_road_ubo.lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}

		DSturnLeft.map(currentImage, &turn_left, 1);
		DSturnLeft.map(currentImage, &carLights_ubo, 2);
		DSturnLeft.map(currentImage, &lights_turn_left_road_ubo, 3);

		RoadUniformBufferObject r_tile{};
		RoadLightsUniformBufferObject tile_lights{};
		for (int i = 0; i < mapIndexes[NONE].size(); i++) {
			int n = mapIndexes[NONE][i].first;
			int m = mapIndexes[NONE][i].second;
			r_tile.mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
							 glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.12f, 0.0f));
			r_tile.mvpMat[i] = vpMat * r_tile.mMat[i];
			r_tile.nMat[i] = glm::inverse(glm::transpose(r_tile.mMat[i]));
			tile_lights.spotLight_lightPosition[i][0] = glm::vec4(glm::vec3(mapLoaded[n][m].pos) + glm::vec3(0.0f, 5.0f, 0.0f), 1.0f);
			tile_lights.spotLight_spotDirection[i][0] = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
			tile_lights.spotLight_lightPosition[i][1] = glm::vec4(glm::vec3(mapLoaded[n][m].pos) + glm::vec3(0.0f, 5.0f, 0.0f), 1.0f);
			tile_lights.spotLight_spotDirection[i][1] = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
		}
		tile_lights.lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		DStile.map(currentImage, &r_tile, 1);
		DStile.map(currentImage, &carLights_ubo, 2);
		DStile.map(currentImage, &tile_lights, 3);

		//Environment
		EnvironmentUniformBufferObject env_ubo{};
		for (int i = 0; i < DSenvironment.size(); i++) {
			for (int j = 0; j < envIndexesPerModel[i].size(); j++) {
				int n = envIndexesPerModel[i][j].first;
				int m = envIndexesPerModel[i][j].second;
				env_ubo.mMat[j] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos);
				env_ubo.mvpMat[j] = vpMat * env_ubo.mMat[j];
				env_ubo.nMat[j] = glm::inverse(glm::transpose(env_ubo.mMat[j]));
			}
			DSenvironment[i].map(currentImage, &env_ubo, 0);
		}

	}
};

// This is the main: probably you do not need to touch this!
int main() {
	A10 app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}