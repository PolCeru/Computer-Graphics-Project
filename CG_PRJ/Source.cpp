#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include <filesystem>
#include <map>
#include <string>
#include <random>
#include <iostream>
#include <vector>
#include <algorithm>

#define MAP_SIZE 11
#define DIRECTIONS 4
#define SCALING_FACTOR 16.0f
#define NUM_CARS 3

//Global
struct GlobalUniformBufferObject {
	//Direct Light
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 viewerPosition;
};

//Car
struct CarUniformBufferObject {
	alignas(16) glm::mat4 mvpMat; //World View Projection Matrix
	alignas(16) glm::mat4 mMat;   //Model/World Matrix
	alignas(16) glm::mat4 nMat;   //Normal Matrix
};

/*struct CarLightsUniformBufferObject {
    // Headlights
    alignas(16) glm::vec3 headlightPosition[NUM_CARS][2];  //left and right
    alignas(16) glm::vec3 headlightDirection[NUM_CARS][2];
    alignas(16) glm::vec4 headlightColor[NUM_CARS][2];

    //Rear lights
    alignas(16) glm::vec3 rearLightPosition[2];  //left and right
    alignas(16) glm::vec3 rearLightDirection[2]; 
    alignas(16) glm::vec4 rearLightColor[2];     
};*/

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

struct Checkpoint{
	glm::vec3 position;
	glm::vec3 pointA;
	glm::vec3 pointB;
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
class CG_PRJ : public BaseProject {
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

	//Car
	DescriptorSetLayout DSLcar;
	VertexDescriptor VDcar;
	Pipeline Pcar;
	std::vector<Model> Mcar; 
	std::vector<DescriptorSet> DScar; 

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


	//Text maker
	TextMaker txt; 

	/******* APP PARAMETERS *******/
	float ar;
	const float FOVy = glm::radians(75.0f);
	const float nearPlane = 0.1f;
	const float farPlane = 500.0f;
	const float baseObjectRotation = 90.0f;

	/******* CAMERA PARAMETERS *******/
	float alpha = M_PI;					// yaw
	float beta = glm::radians(5.0f);    // pitch
	float camDist = 7.0f;				// distance from the target
	float camHeight = 2.0f;				// height from the target
	const float lambdaCam = 10.0f;      // damping factor for the camera

	glm::vec3 camPos = glm::vec3(0.0, camHeight, camDist);				//Camera Position (-l/+r, -d/+u, b/f)
	glm::mat4 viewMatrix = glm::translate(glm::mat4(1), -camPos);		//View Matrix setup
	const glm::vec3 upVector = glm::vec3(0, 1, 0);						//Up Vector

	/******* CARS PARAMETERS *******/
	float initialRotation = 0.0f;

	const float ROT_SPEED = glm::radians(120.0f);
	const float MOVE_SPEED = 2.0f;
	const float carAcceleration = 8.0f;						// [m/s^2]
	const float brakingStrength = 30.0f;
	const float gravity = 9.81f;								// [m/s^2]
	const float friction = 0.7f * gravity;
	float carSteeringSpeed = glm::radians(75.0f);
	const float carDamping = 5.0f;
	std::vector<glm::vec3> startingCarPos;
	std::vector<glm::vec3> updatedCarPos;
	std::vector<float> carVelocity;	
	std::vector<float> steeringAng; 

	// Assume initialRotation is the rotation applied to the car model at spawn, represented as a quaternion
	glm::quat initialRotationQuat;

	std::map<int, std::vector<bool>> rightTurnsCrossed; 
	std::map<int, std::vector<bool>> leftTurnsCrossed;
	std::map<int, int> straightTurnCrossed; 

	/******* MAP PARAMETERS *******/
	nlohmann::json mapFile;
	const int MAP_CENTER = MAP_SIZE / 2;
	int maxLaps = 2;
	std::vector<std::vector<RoadPosition>> mapLoaded;
	std::vector<std::vector<std::pair<int, int>>> mapIndexes; // 0: STRAIGHT, 1: LEFT, 2: RIGHT
	std::map<int, Checkpoint> checkpoints;

	/************ DAY PHASES PARAMETERS *****************/
	int scene = 0; 
	float turningTime = 0.0f; 
	float sun_cycle_duration = 120.0f;
	float daily_phase_duration = sun_cycle_duration / 3.0f; 
	float rad_per_sec = M_PI / sun_cycle_duration;
	float timeScene = 0.0f; 
	float timeFactor = 0.0f; 
	glm::vec4 sunriseColor = glm::vec4(1.0f, 0.39f, 0.28f, 1.0f);
	glm::vec4 dayColor = glm::vec4(0.75f, 0.65f, 0.3f, 1.0f);
	glm::vec4 sunsetColor = glm::vec4(0.85f, 0.5f, 0.2f, 1.0f);
	glm::vec4 moonColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	glm::vec3 startingColor = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 finalColor = glm::vec3(0.0f, 0.0f, 0.0f);

	/******* PLAYER PARAMETERS *******/
	int currentCheckpoint = 0;
	int currentLap = 0;
	int counter = 0;
	const int player_car = 0;


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

	//Helper Functions
	std::unordered_map<std::string, RoadType> stringToEnumRT = {
		{"STRAIGHT", RoadType::STRAIGHT},
		{"LEFT", RoadType::LEFT},
		{"RIGHT", RoadType::RIGHT},
		{"NONE", RoadType::NONE},
	};

	RoadType getRTEnumFromString(const std::string& enumString) {
		auto it = stringToEnumRT.find(enumString);
		if (it != stringToEnumRT.end()) {
			return it->second;
		} else {
			throw std::invalid_argument("Invalid enum string: " + enumString);
		}
	}

	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		startingCarPos.resize(NUM_CARS);
		updatedCarPos.resize(NUM_CARS);
		carVelocity.resize(NUM_CARS);
		steeringAng.resize(NUM_CARS);
		for (int i = 0; i < startingCarPos.size(); i++) {
			carVelocity[i] = 0.0f;
			steeringAng[i] = 0.0f;
		}
		InitDSL();
		InitVD();
		InitPipelines();
		InitModels();

		//Map Grid Initialization
		mapFile = LoadMapFile();
		LoadMap(mapFile);
		//Environment models
		/*readModels(envModelsPath);
		Menv.resize(envFileNames.size());
		for (const auto& [key, value] : envFileNames) {
			Menv[key].init(this, &VDenv, value, MGCG);
		}
		InitEnvironment();*/

		//Textures
		LoadTextures();

		UpdatePools();
	}

	//Descriptor Set Layout
	void InitDSL()
	{
		//Global
		DSLGlobal.init(this, {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1 }
		});

		//Skybox
		DSLSkyBox.init(this, {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(skyBoxUniformBufferObject), 1 },
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1 },
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1 }
		});

		//Road
		DSLroad.init(this, {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1 },
			{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(RoadUniformBufferObject), 1 },
			//{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(CarLightsUniformBufferObject), 1 },
			{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(RoadLightsUniformBufferObject), 1 }
		});

		//Car
		DSLcar.init(this, {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(CarUniformBufferObject), 1 },
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1 }
			//{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(CarLightsUniformBufferObject), 1 }
		});

		/*DSLenvironment.init(this, {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(EnvironmentUniformBufferObject), 1 },
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1 }
		});*/
	}

	//Vertex Descriptor
	void InitVD()
	{
		//Skybox
		VDSkyBox.init(this, {
			{ 0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX }
		}, {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos), sizeof(glm::vec3), POSITION }
		});

		//Road
		VDroad.init(this, {
			{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
		}, {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION },
			{ 0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv), sizeof(glm::vec2), UV },
			{ 0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal), sizeof(glm::vec3), NORMAL },
		});

		//Car
		VDcar.init(this, {
			{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
		}, {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION },
			{ 0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv), sizeof(glm::vec2), UV },
			{ 0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal), sizeof(glm::vec3), NORMAL },
		});

		//Environment
		/*VDenv.init(this, {
			{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
		}, {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION },
			{ 0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv), sizeof(glm::vec2), UV },
			{ 0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal), sizeof(glm::vec3), NORMAL },
		});*/
	}

	//Pipelines
	void InitPipelines()
	{
		PSkyBox.init(this, &VDSkyBox, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", { &DSLSkyBox });
		PSkyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false);
		Proad.init(this, &VDroad, "shaders/RoadVert.spv", "shaders/RoadFrag.spv", { &DSLGlobal, &DSLroad });
		Proad.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Pcar.init(this, &VDcar, "shaders/CarVert.spv", "shaders/CarFrag.spv", { &DSLGlobal, &DSLcar });
		/*Penv.init(this, &VDenv, "shaders/EnvVert.spv", "shaders/EnvFrag.spv", {&DSLGlobal, &DSLenvironment});
		Penv.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);*/
	}

	//Models
	void InitModels()
	{
		MSkyBox.init(this, &VDSkyBox, "models/SkyBoxCube.obj", OBJ);

		Mcar.resize(NUM_CARS); 

		for (int i = 0; i < Mcar.size(); i++) {
			Mcar[i].init(this, &VDcar, "models/cars/car_" + std::to_string(i) + ".mgcg", MGCG);
		}

		MstraightRoad.init(this, &VDroad, "models/road/straight.mgcg", MGCG);
		MturnLeft.init(this, &VDroad, "models/road/turn.mgcg", MGCG);
		MturnRight.init(this, &VDroad, "models/road/turn.mgcg", MGCG);
		Mtile.init(this, &VDroad, "models/road/green_tile.mgcg", MGCG);
	}
	
	//Initialize the mapLoaded and mapIndexes with the default values
	void InitMap()
	{
		mapIndexes.resize(DIRECTIONS);
		mapLoaded.resize(MAP_SIZE, std::vector<RoadPosition>(MAP_SIZE));
		for (int i = 0; i < mapLoaded.size(); i++) {
			for (int j = 0; j < mapLoaded[i].size(); j++) {
				std::pair <int, int> index = std::make_pair(i, j);
				float x = SCALING_FACTOR * (j - MAP_CENTER);
				float z = SCALING_FACTOR * (i - MAP_CENTER);
				int type = RoadType::NONE;
				mapLoaded[i][j].pos = glm::vec3(x, 0.0f, z);
				mapLoaded[i][j].type = type;
				mapLoaded[i][j].rotation = 0.0f;
				mapIndexes[type].push_back(index);
			}
		}
	}

	//Reads the models from the environment folder
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

	//Loads the JSON
	nlohmann::json LoadMapFile(){
		nlohmann::json json;

		std::ifstream infile("config/map_ina.json");
		if (!infile.is_open()) {
			std::cerr << "Error opening file!" << std::endl;
			exit(1);
		}

		// Parse the JSON content
		infile >> json;
		infile.close();

		return json;
	}

	//Initialize the mapLoaded and mapIndexes
	void LoadMap(nlohmann::json& json)
	{
		InitMap();
		std::pair<int, int> previousItemIndex = std::make_pair(json["start"]["row"], json["start"]["col"]);
		std::cout << json["map"][1]["col"] << std::endl; 
		auto col = json["map"][1]["col"]; 
		initialRotation = ((int)col - previousItemIndex.second > 0) ? 270.0f : ((int)col - previousItemIndex.second < 0) ? 90.0f : 0.0f; // Set the initial rotation 
		mapLoaded[previousItemIndex.first][previousItemIndex.second].rotation = initialRotation;

		initialRotationQuat = glm::quat(glm::vec3(0.0f, glm::radians(initialRotation), 0.0f)); //Represents the rotation applied to the car model at spawn
		int lastCpIndex = 0;
		for (const auto& [jsonKey, jsonValues] : json.items()) {
			
			if (jsonKey == "map"){
				for (const auto& [mapKey, mapValues] : jsonValues.items()) {
					std::pair <int, int> index = std::make_pair(mapValues["row"], mapValues["col"]);
					RoadType type = getRTEnumFromString(mapValues["type"]);
					// Add the road piece to the map
					mapLoaded[index.first][index.second].type = type;

					// Add the index to the corresponding type and remove the index from the NONE type
					mapIndexes[type].push_back(index);
					mapIndexes[RoadType::NONE].erase(std::remove(mapIndexes[RoadType::NONE].begin(), mapIndexes[RoadType::NONE].end(), index), mapIndexes[RoadType::NONE].end());

					rotationHandler(previousItemIndex, index, type);
					previousItemIndex = index;
				}
			} 
			else if (jsonKey == "checkpoints"){
				if (jsonValues.size() != 0) {
					for (const auto& [cpKey, cpValues] : jsonValues.items()) {
						std::pair <int, int> checkpointPosIndex = std::make_pair(cpValues["row"], cpValues["col"]);
						lastCpIndex = std::stoi(cpKey);
						glm::vec3 position = mapLoaded[checkpointPosIndex.first][checkpointPosIndex.second].pos;
						float rotation = mapLoaded[checkpointPosIndex.first][checkpointPosIndex.second].rotation;
						initCheckpoint(position, rotation, lastCpIndex);
					}
				}
			}
			else if (jsonKey == "start"){
				std::pair <int, int> startPosIndex = std::make_pair(jsonValues["row"], jsonValues["col"]);
				startingCarPos[player_car] = mapLoaded[startPosIndex.first][startPosIndex.second].pos;
				for (int i = 0; i < startingCarPos.size(); i++) {
					if (i != player_car) {
						startingCarPos[i] = glm::vec3(startingCarPos[player_car].x, startingCarPos[player_car].y, startingCarPos[player_car].z + (4.0f * i));
					}
				}
				camPos = glm::vec3(startingCarPos[player_car].x, camHeight, startingCarPos[player_car].z - camDist);
				updatedCarPos = startingCarPos;
				
			}
			else if (jsonKey == "end") {
				std::pair <int, int> endPosIndex;
				int cpIndex;
				if (jsonValues == nullptr) {
					endPosIndex = std::make_pair(json["map"].back()["row"], json["map"].back()["col"]);
					lastCpIndex++;
				}
				else {
					endPosIndex = std::make_pair(jsonValues["row"], jsonValues["col"]);
					std::cout << lastCpIndex << std::endl;
					lastCpIndex++;
					maxLaps = 1;
				}
				glm::vec3 position = mapLoaded[endPosIndex.first][endPosIndex.second].pos;
				float rotation = mapLoaded[endPosIndex.first][endPosIndex.second].rotation;
				initCheckpoint(position, rotation, lastCpIndex);
			}
			for (int i = 0; i < NUM_CARS; i++) {
				rightTurnsCrossed[i].resize(mapIndexes[RIGHT].size()); 
				leftTurnsCrossed[i].resize(mapIndexes[LEFT].size()); 
				for (int j = 0; j < rightTurnsCrossed[i].size(); j++) {
					rightTurnsCrossed[i][j] = false; 
				}
				for (int j = 0; j < leftTurnsCrossed[i].size(); j++) {
					leftTurnsCrossed[i][j] = false;
				}
				straightTurnCrossed[i] = 0; 
			}
		}
	}

	void initCheckpoint(glm::vec3& checkpointPos, float rotation, int id)
	{
		/*/glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
		checkpoints[id].position = checkpointPos;
		
		checkpoints[id].pointA = checkpointPos - glm::vec3(rotationMatrix * glm::vec4(6.0f, 0.0f, 0.0f, 1.0f));
		checkpoints[id].pointB = checkpointPos + glm::vec3(rotationMatrix * glm::vec4(6.0f, 0.0f, 0.0f, 1.0f));*/

		checkpoints[id].position = checkpointPos;
		checkpoints[id].pointA = checkpointPos - glm::vec3(6.0f, 0.0f, 6.0f);
		checkpoints[id].pointB = checkpointPos + glm::vec3(6.0f, 0.0f, 6.0f);
	}

	//Handles the rotation of the road pieces
	void rotationHandler(std::pair<int, int>& previousItemIndex, std::pair<int, int>& index, int type)
	{
		//if the current item is a turn depending on the current rotation and the type of the turn, the rotation is updated
		int dRow = previousItemIndex.first - index.first;    // x difference
		int dCol = previousItemIndex.second - index.second;  // y difference

		if (dRow != 0 && dCol != 0) {
			// Row and Column are both different from the previous road block, which is an error
			std::cerr << "Invalid map configuration, Row and Column are both different from the previous road block"
				<< ((abs(dRow) > abs(dCol)) ? " (Col problem)" : " (Row problem)") << std::endl;
			std::cerr << "The road blocks are not connected or in wrong order in the map file" << std::endl;
			std::cerr << "Previous road block: " << previousItemIndex.first << " " << previousItemIndex.second << std::endl;
			std::cerr << "Current road block: " << index.first << " " << index.second << std::endl;
			std::cerr << "Rotation defaulted to 0.0f" << std::endl;
		}
		else {
			if (dRow > 0) 
				mapLoaded[index.first][index.second].rotation = (type == LEFT) ? 270.0f : (type == RIGHT) ? 0.0f : 0.0f;
			else if (dRow < 0) 
				mapLoaded[index.first][index.second].rotation = (type == LEFT) ? 90.0f : (type == RIGHT) ? 180.0f : 0.0f;
			else if (dCol > 0) 
				mapLoaded[index.first][index.second].rotation = (type == LEFT) ? 0.0f : (type == RIGHT) ? 90.0f : 90.0f;
			else if (dCol < 0) 
				mapLoaded[index.first][index.second].rotation = (type == LEFT) ? 180.0f : (type == RIGHT) ? 270.0f : 90.0f;
		}
	}

	//Environment
	void InitEnvironment()
	{
		std::random_device rd;										//Obtain a random number from hardware
		std::mt19937 gen(rd());										//Seed the generator
		std::uniform_int_distribution<> distr(-5, Menv.size() - 1);	//Define the range (negative values are for blank tiles)

		//Random distribution of environment models on the map
		envIndexesPerModel.resize(Menv.size());
		for (int i = 0; i < mapIndexes[RoadType::NONE].size(); i++) {
			int modelNumber = distr(gen);
			if (modelNumber >= 0) {
				std::pair <int, int> index = mapIndexes[RoadType::NONE][i];
				envIndexesPerModel[modelNumber].push_back(index);
			}
		}
	}

	//Textures
	void LoadTextures()
	{
		TSkyBox.init(this, "textures/starmap_g4k.jpg");
		Tenv.init(this, "textures/Textures_City.png");
		TStars.init(this, "textures/constellation_figures.png");
		Tclouds.init(this, "textures/Clouds.jpg");
		Tsunrise.init(this, "textures/SkySunrise.png");
		Tday.init(this, "textures/SkyDay.png");
		Tsunset.init(this, "textures/SkySunset.png");
	}

	// Update the Descriptor Sets Pools
	void UpdatePools()
	{
		DPSZs.uniformBlocksInPool = 1 + Mcar.size() + 12 + 2 + Menv.size();				// summation of (#ubo * #DS) for each DSL
		DPSZs.texturesInPool = 2 + 4 + Mcar.size() + Menv.size();						// summation of (#texure * #DS) for each DSL
		DPSZs.setsInPool = 6 + Mcar.size() + Menv.size();								// summation of #DS for each DSL

		std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
		std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
		std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";
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

		DScar.resize(NUM_CARS); 
		for (int i = 0; i < DScar.size(); i++) {
			DScar[i].init(this, &DSLcar, {&Tenv});
		}

		/*DSenvironment.resize(Menv.size());
		for (int i = 0; i < DSenvironment.size(); i++) {
			DSenvironment[i].init(this, &DSLenvironment, { &Tenv });
		}*/
		
		//Pipeline Creation
		PSkyBox.create();
		Proad.create();
		Pcar.create();
		//Penv.create();
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		//Pipelines Cleanup
		PSkyBox.cleanup();
		Proad.cleanup();
		Pcar.cleanup();
		//Penv.cleanup();

		//Descriptor Set Cleanup
		DSGlobal.cleanup();
		DSSkyBox.cleanup();
		DSstraightRoad.cleanup();
		DSturnLeft.cleanup();
		DSturnRight.cleanup();
		DStile.cleanup();
		for (int i = 0; i < DScar.size(); i++) {
			DScar[i].cleanup();
		}
		/*for (int i = 0; i < DSenvironment.size(); i++) {
			DSenvironment[i].cleanup();
		}*/
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
		for (int i = 0; i < Mcar.size(); i++) {
			Mcar[i].cleanup();
		}
		/*for (int i = 0; i < Menv.size(); i++) {
			Menv[i].cleanup();
		}*/

		//Descriptor Set Layouts Cleanup
		DSLGlobal.cleanup();
		DSLSkyBox.cleanup();
		DSLroad.cleanup();
		DSLcar.cleanup();
		//DSLenvironment.cleanup();

		//Pipelines destruction
		PSkyBox.destroy();
		Proad.destroy();
		Pcar.destroy();
		//Penv.destroy();
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
		DSGlobal.bind(commandBuffer, Pcar, 0, currentImage);
		for (int i = 0; i < Mcar.size(); i++) {
			DScar[i].bind(commandBuffer, Pcar, 1, currentImage);
			Mcar[i].bind(commandBuffer);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mcar[i].indices.size()), 1, 0, 0, 0);
		}

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
		/*Penv.bind(commandBuffer);
		for (int i = 0; i < Menv.size(); i++) {
			Menv[i].bind(commandBuffer);
			DSGlobal.bind(commandBuffer, Penv, 0, currentImage);
			DSenvironment[i].bind(commandBuffer, Penv, 1, currentImage);
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Menv[i].indices.size()), static_cast<uint32_t>(envIndexesPerModel[i].size()), 0, 0, 0);
		}*/
	}

	// Function to check if the car is crossing the checkpoint
	bool IsBetweenPoints(const glm::vec3& carPos, const Checkpoint& checkpoint) {
		float tolerance = 0.2f;
		bool withinX (carPos.x >= (glm::min(checkpoint.pointA.x, checkpoint.pointB.x) - tolerance) && carPos.x <= (glm::max(checkpoint.pointA.x, checkpoint.pointB.x) + tolerance));
		bool withinZ (carPos.z >= (glm::min(checkpoint.pointA.z, checkpoint.pointB.z) - tolerance) && carPos.z <= (glm::max(checkpoint.pointA.z, checkpoint.pointB.z) + tolerance));

		/*std::cout << "X: " << carPos.x << " >= " << glm::min(checkpoint.pointA.x, checkpoint.pointB.x) - tolerance << " && " << carPos.x << " <= " << glm::max(checkpoint.pointA.x, checkpoint.pointB.x) + tolerance << std::endl;
		std::cout << "Z: " << carPos.z << " >= " << glm::min(checkpoint.pointA.z, checkpoint.pointB.z) - tolerance << " && " << carPos.z << " <= " << glm::max(checkpoint.pointA.z, checkpoint.pointB.z) + tolerance << std::endl;

		std::cout << withinX << " " << withinZ << std::endl;*/

		return withinX && withinZ;
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {

		// Parameters for the SixAxis
		float deltaT;					// Time between frames [seconds]
		glm::vec3 m = glm::vec3(0.0f);  // Movement
		glm::vec3 r = glm::vec3(0.0f);  // Rotation
		bool fire = false;				// Button pressed
		getSixAxis(deltaT, m, r, fire);

		//Matrices setup 
		glm::mat4 pMat = glm::perspective(FOVy, ar, nearPlane, farPlane);	//Projection Matrix
		pMat[1][1] *= -1;													//Flip Y
		glm::mat4 vpMat;													//View Projection Matrix

		if (currentLap < maxLaps) {
			CarsMotionHandler(deltaT, m);
		}

		//Walk model procedure 
		CameraPositionHandler(r, deltaT, m, vpMat, pMat);		

		//Checkpoint handling
		if (IsBetweenPoints(updatedCarPos[player_car], checkpoints[currentCheckpoint])) {
			currentCheckpoint++;
			if (currentCheckpoint == checkpoints.size()) {
				currentCheckpoint = 0;
				currentLap++;
			}
		}

		//checkpoint Debug
		counter++;
		if (counter % 25 == 0){
			std::cout << "Checkpoint: " << currentCheckpoint << std::endl;

			printVec3("Car Position", updatedCarPos[player_car]);
			printVec3("Checkpoint Position", checkpoints[currentCheckpoint].position);
			printVec3("Point A", checkpoints[currentCheckpoint].pointA);
			printVec3("Point B", checkpoints[currentCheckpoint].pointB);

			std::cout << "Lap: " << currentLap << " Checkpoint: " << currentCheckpoint << std::endl;
			counter = 0;
		}
	
		// Scenery change and update
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

		if (scene != 3) 
			g_ubo.lightDir = glm::vec3(0.0f, sin(glm::radians(180.0f) - rad_per_sec * turningTime), cos(glm::radians(180.0f) - rad_per_sec * turningTime));
		else
			g_ubo.lightDir = glm::vec3(0.0f, sin(glm::radians(180.0f) - rad_per_sec * (turningTime - sun_cycle_duration)), cos(glm::radians(180.0f) - rad_per_sec * (turningTime - sun_cycle_duration)));

		timeScene = turningTime - scene * daily_phase_duration;
		timeFactor = timeScene / daily_phase_duration;

		switch (scene) {
			case 0: //from sunrise to day
				startingColor = glm::vec3(sunriseColor); 
				finalColor = glm::vec3(dayColor); 
				break; 
			case 1: // from day to sunset
				startingColor = glm::vec3(dayColor); 
				finalColor = glm::vec3(sunsetColor); 
				break;
			case 2: //from sunset to night
				startingColor = glm::vec3(sunsetColor); 
				finalColor = glm::vec3(0.0f, 0.0f, 0.0f); 
				break; 
			default: // night
				startingColor = glm::vec3(moonColor); 
				finalColor = glm::vec3(moonColor);
				break; 
		}
		g_ubo.lightColor = glm::vec4(startingColor * (1 - timeFactor) + finalColor * timeFactor, 1.0f);
		g_ubo.viewerPosition = glm::vec3(glm::inverse(viewMatrix) * glm::vec4(0, 0, 0, 1)); // would dampedCam make sense?
		DSGlobal.map(currentImage, &g_ubo, 0);

		//Object Uniform Buffer creation
		//SkyBox
		skyBoxUniformBufferObject sb_ubo{};
		sb_ubo.mvpMat = pMat * glm::mat4(glm::mat3(viewMatrix)); //Remove Translation part of ViewMatrix, take only Rotation part and applies Projection
		DSSkyBox.map(currentImage, &sb_ubo, 0);

		//Player Car
		CarUniformBufferObject car_ubo_zero{};
		car_ubo_zero.mMat = glm::translate(glm::mat4(1.0f), updatedCarPos[0]) *
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f + initialRotation) + steeringAng[0], glm::vec3(0, 1, 0));
		car_ubo_zero.mvpMat = vpMat * car_ubo_zero.mMat;
		car_ubo_zero.nMat = glm::inverse(glm::transpose(car_ubo_zero.mMat));
		DScar[0].map(currentImage, &car_ubo_zero, 0);

		CarUniformBufferObject car_ubo_one{};
		car_ubo_one.mMat = glm::translate(glm::mat4(1.0f), updatedCarPos[1]) *
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f + initialRotation) + steeringAng[1], glm::vec3(0, 1, 0));
		car_ubo_one.mvpMat = vpMat * car_ubo_one.mMat;
		car_ubo_one.nMat = glm::inverse(glm::transpose(car_ubo_one.mMat));
		DScar[1].map(currentImage, &car_ubo_one, 0);

		CarUniformBufferObject car_ubo_two{};
		car_ubo_two.mMat = glm::translate(glm::mat4(1.0f), updatedCarPos[2]) *
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f + initialRotation) + steeringAng[2], glm::vec3(0, 1, 0));
		car_ubo_two.mvpMat = vpMat * car_ubo_two.mMat;
		car_ubo_two.nMat = glm::inverse(glm::transpose(car_ubo_two.mMat));
		DScar[2].map(currentImage, &car_ubo_two, 0);



		/*CarLightsUniformBufferObject carLights_ubo{};
		for (int j = 0; j < NUM_CARS; j++){
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), steeringAng[j], glm::vec3(0.0f, 1.0f, 0.0f));
			for (int i = 0; i < 2; i++) {
				glm::vec3 lightsOffset = glm::vec3((i == 0) ? -0.5f : 0.5f, 0.6f, -1.5f);
				carLights_ubo.headlightPosition[j][i] = updatedCarPos + glm::vec3(rotationMatrix * glm::vec4(lightsOffset, 1.0f));
				carLights_ubo.headlightDirection[j][i] = glm::vec3(rotationMatrix * glm::vec4(0.0f, -0.5f, -1.0f, 0.0f)); //pointing forward
				if (scene == 3) {
					carLights_ubo.headlightColor[j][i] = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f); //white
				}
				else {
					carLights_ubo.headlightColor[j][i] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
				}

				lightsOffset = glm::vec3((i == 0) ? -0.55f : 0.55f, 0.6f, 1.9f);
				carLights_ubo.rearLightPosition[j][i] = updatedCarPos + glm::vec3(rotationMatrix * glm::vec4(lightsOffset, 1.0f));
				carLights_ubo.rearLightDirection[j][i] = glm::vec3(rotationMatrix * glm::vec4(0.0f, -0.5f, 1.0f, 0.0f)); //pointing backwards
				if (scene == 3) {
					carLights_ubo.rearLightColor[j][i] = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f); //red
				}
				else {
					carLights_ubo.rearLightColor[j][i] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
				}
			}
			DScar[j].map(currentImage, &carLights_ubo, 2);
		}*/

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
			
			bool oneCondition = false;
			bool m_oneCondition = false;
			int directions[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} }; // Up, Down, Left, Right

			for (int i = 0; i < 4; i++) {
				int newN = n + directions[i][0];
				int newM = m + directions[i][1];

				// Check if the neighboring cell has type 1 or 2
				if (mapLoaded[newN][newM].type == 1 || mapLoaded[newN][newM].type == 2) {
					// Identify the condition based on the direction
					if (directions[i][0] >= 0 && directions[i][1] >= 0) {
						oneCondition = true;
					} else if (directions[i][0] <= 0 && directions[i][1] <= 0) {
						m_oneCondition = true;
					}
				}
			}

			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0));
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) * rotation;

			// Spot positions //0 middle, 1 previous, 2 next (the one furthest from the model)
			lights_straight_road_ubo.spotLight_lightPosition[i][0] = transform * glm::vec4(-4.9f, 4.9f, -0.2f, 1.0f);
			lights_straight_road_ubo.spotLight_spotDirection[i][0] = rotation * glm::vec4(0.4f, -1.0f, 0.0f, 1.0f);
			
			if (!oneCondition){
				lights_straight_road_ubo.spotLight_lightPosition[i][1] = transform * glm::vec4(4.9f, 4.9f, 7.8f, 1.0f);
				lights_straight_road_ubo.spotLight_spotDirection[i][1] = rotation * glm::vec4(-0.4f, -1.0f, 0.0f, 1.0f);
			}

			if (!m_oneCondition){
				lights_straight_road_ubo.spotLight_lightPosition[i][2] = transform * glm::vec4(4.9f, 4.9f, -7.8f, 1.0f);
				lights_straight_road_ubo.spotLight_spotDirection[i][2] = rotation * glm::vec4(-0.4f, -1.0f, 0.0f, 1.0f);
			} 
			
		}
		if (scene == 3) {
			lights_straight_road_ubo.lightColorSpot = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f);
		}
		else {
			lights_straight_road_ubo.lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		DSstraightRoad.map(currentImage, &straight_road_ubo, 1);
		//DSstraightRoad.map(currentImage, &carLights_ubo, 2);
		DSstraightRoad.map(currentImage, &lights_straight_road_ubo, 2);

		RoadUniformBufferObject turn_right{};
		RoadLightsUniformBufferObject lights_turn_right_road_ubo{};
		for (int i = 0; i < mapIndexes[RIGHT].size(); i++) {
			int n = mapIndexes[RIGHT][i].first;
			int m = mapIndexes[RIGHT][i].second;
			turn_right.mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
								 glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation + baseObjectRotation), glm::vec3(0, 1, 0));
			turn_right.mvpMat[i] = vpMat * turn_right.mMat[i];
			turn_right.nMat[i] = glm::inverse(glm::transpose(turn_right.mMat[i]));		
			
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0));
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) * rotation;

			// Spot positions
			lights_turn_right_road_ubo.spotLight_lightPosition[i][0] = transform * glm::vec4(-4.85f, 4.9f, -5.9f, 1.0f);

			// Spot directions
			lights_turn_right_road_ubo.spotLight_spotDirection[i][0] = rotation * glm::vec4(0.4f, -1.0f, 0.4f, 1.0f);
		}

		if (scene == 3) {
			lights_turn_right_road_ubo.lightColorSpot = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f);
		}
		else {
			lights_turn_right_road_ubo.lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		DSturnRight.map(currentImage, &turn_right, 1);
		//DSturnRight.map(currentImage, &carLights_ubo, 2);
		DSturnRight.map(currentImage, &lights_turn_right_road_ubo, 2);

		RoadUniformBufferObject turn_left{};
		RoadLightsUniformBufferObject lights_turn_left_road_ubo{}; 
		for (int i = 0; i < mapIndexes[LEFT].size(); i++) {
			int n = mapIndexes[LEFT][i].first;
			int m = mapIndexes[LEFT][i].second;
			turn_left.mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
								glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation + baseObjectRotation), glm::vec3(0, 1, 0));
			turn_left.mvpMat[i] = vpMat * turn_left.mMat[i];
			turn_left.nMat[i] = glm::inverse(glm::transpose(turn_left.mMat[i]));

			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation - 90.0f), glm::vec3(0, 1, 0));
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) * rotation;

			// Spot positions
			lights_turn_left_road_ubo.spotLight_lightPosition[i][0] = transform * glm::vec4(-5.8f, 5.0f, 4.65f, 1.0f);

			// Spot directions
			lights_turn_left_road_ubo.spotLight_spotDirection[i][0] = rotation * glm::vec4(0.4f, -1.0f, -0.4f, 1.0f);
		}

		if (scene == 3) {
			lights_turn_left_road_ubo.lightColorSpot = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f);
		}
		else{
			lights_turn_left_road_ubo.lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}

		DSturnLeft.map(currentImage, &turn_left, 1);
		//DSturnLeft.map(currentImage, &carLights_ubo, 2);
		DSturnLeft.map(currentImage, &lights_turn_left_road_ubo, 2);

		RoadUniformBufferObject r_tile{};
		RoadLightsUniformBufferObject tile_lights{};
		for (int i = 0; i < mapIndexes[NONE].size(); i++) {
			int n = mapIndexes[NONE][i].first;
			int m = mapIndexes[NONE][i].second;
			r_tile.mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos);
			r_tile.mvpMat[i] = vpMat * r_tile.mMat[i];
			r_tile.nMat[i] = glm::inverse(glm::transpose(r_tile.mMat[i]));
			tile_lights.spotLight_lightPosition[i][0] = glm::vec4(glm::vec3(mapLoaded[n][m].pos) + glm::vec3(0.0f, 5.0f, 0.0f), 1.0f);
			tile_lights.spotLight_lightPosition[i][1] = glm::vec4(glm::vec3(mapLoaded[n][m].pos) + glm::vec3(0.0f, 5.0f, 0.0f), 1.0f);
			tile_lights.spotLight_spotDirection[i][0] = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
			tile_lights.spotLight_spotDirection[i][1] = glm::vec4(0.0f, -1.0f, 0.0f, 1.0f);
		}
		tile_lights.lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		DStile.map(currentImage, &r_tile, 1);
		//DStile.map(currentImage, &carLights_ubo, 2);
		DStile.map(currentImage, &tile_lights, 2);

		//Environment
		/*EnvironmentUniformBufferObject env_ubo{};
		for (int i = 0; i < DSenvironment.size(); i++) {
			for (int j = 0; j < envIndexesPerModel[i].size(); j++) {
				int n = envIndexesPerModel[i][j].first;
				int m = envIndexesPerModel[i][j].second;
				env_ubo.mMat[j] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos)*
								  glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, +0.2f, 0.0f));;
				env_ubo.mvpMat[j] = vpMat * env_ubo.mMat[j];
				env_ubo.nMat[j] = glm::inverse(glm::transpose(env_ubo.mMat[j]));
			}
			DSenvironment[i].map(currentImage, &env_ubo, 0);
		}*/

	}
	
	//Defines the dynamics of the car movement and updates the car position
	void CarsMotionHandler(float deltaT, glm::vec3& m)
	{
		bool handbrake = false;
		if (glfwGetKey(window, GLFW_KEY_SPACE)) {
			handbrake = true;
			if (carVelocity[player_car] > 0) {
				carVelocity[player_car] -= brakingStrength * 2 * deltaT;
			}
		}
		(handbrake) ? carSteeringSpeed = glm::radians(90.0f) : carSteeringSpeed = glm::radians(60.0f);

		// Handle acceleration/braking
		if (m.z < 0) { // w pressed
			if (carVelocity[player_car] >= 0) {
				carVelocity[player_car] += carAcceleration * deltaT;
				carVelocity[player_car] = glm::min(carVelocity[player_car], 70.0f);
			}
			else {
				carVelocity[player_car] += brakingStrength * deltaT;
			}
		}
		else if (m.z > 0) { // s pressed
			if (carVelocity[player_car] > 0) { // car is moving forward, decelerate
				carVelocity[player_car] -= brakingStrength * deltaT;
			}
			else { // car is moving backwards, accelerate in the opposite direction
				m.x *= -1;
				carVelocity[player_car] -= carAcceleration * deltaT;
				carVelocity[player_car] = glm::max(carVelocity[player_car], -15.0f);
			}
		}
		else { // no acceleration or deceleration
			if (carVelocity[player_car] > 0.0f) {
				carVelocity[player_car] -= friction * deltaT;
				carVelocity[player_car] = glm::max(carVelocity[player_car], 0.0f);
			}
			else if (carVelocity[player_car] < 0.0f) {
				m.x *= -1;
				carVelocity[player_car] += friction * deltaT;
				carVelocity[player_car] = glm::min(carVelocity[player_car], 0.0f);
			}
		}

		// Handle steering
		if (carVelocity[player_car] != 0.0f) {
			steeringAng[player_car] += -m.x * carSteeringSpeed * deltaT;
		}

		// Combine the initial rotation with the current steering angle
		glm::quat steeringRotation = glm::angleAxis(steeringAng[player_car], glm::vec3(0.0f, 1.0f, 0.0f));
		glm::quat totalRotation = initialRotationQuat * steeringRotation;

		glm::vec3 forwardDir = totalRotation * glm::vec3(0.0f, 0.0f, -1.0f);	//the forward direction in global space
		startingCarPos[player_car] += forwardDir * carVelocity[player_car] * deltaT;
		updatedCarPos[player_car] = updatedCarPos[player_car] * std::exp(-carDamping * deltaT) + startingCarPos[player_car] * (1 - std::exp(-carDamping * deltaT));

		float minBoundary = -SCALING_FACTOR * MAP_CENTER + 0.1f;
		float maxBoundary = SCALING_FACTOR * MAP_CENTER - 0.1f;

		// Clamp the x and z positions within the boundaries
		updatedCarPos[player_car].x = glm::clamp(updatedCarPos[player_car].x, minBoundary, maxBoundary);
		updatedCarPos[player_car].z = glm::clamp(updatedCarPos[player_car].z, minBoundary, maxBoundary);

		for (int i = 0; i < NUM_CARS; i++) {
			if (i != player_car) {
				carVelocity[i] += carAcceleration * deltaT;
				carVelocity[i] = glm::min(carVelocity[i], 70.0f - (10.0f * i));
				if (carInTurnRight(i, deltaT)) {
					steeringAng[i] -= glm::radians(90.0f); 
				}
				else if (carInTurnLeft(i, deltaT)) {
					steeringAng[i] += glm::radians(90.0f);
				}
				else {
					straightTurnCrossed[i] += 1; 
				}
				if (carFinishedLap(i)) {
					cleanCarLapData(i); 
					
				}
				steeringRotation = glm::angleAxis(steeringAng[i], glm::vec3(0.0f, 1.0f, 0.0f));
				totalRotation = initialRotationQuat * steeringRotation;
				forwardDir = totalRotation * glm::vec3(0.0f, 0.0f, -1.0f);
				startingCarPos[i] += forwardDir * carVelocity[i] * deltaT;
				updatedCarPos[i] = updatedCarPos[i] * std::exp(-carDamping * deltaT) + startingCarPos[i] * (1 - std::exp(-carDamping * deltaT));
			}
		}
	}

	bool carInTurnRight(int carIndex, float deltaT) {
		int n; 
		int m; 
		float tollerance = 2.0f * carVelocity[carIndex] * deltaT;
		for (int i = 0; i < mapIndexes[RIGHT].size(); i++) {
			if (!rightTurnsCrossed[carIndex][i]) {
				n = mapIndexes[RIGHT][i].first;
				m = mapIndexes[RIGHT][i].second;
				bool checkOnX = (updatedCarPos[carIndex].x >= mapLoaded[n][m].pos.x - tollerance) && (updatedCarPos[carIndex].x <= mapLoaded[n][m].pos.x + tollerance);
				bool checkOnZ = (updatedCarPos[carIndex].z >= mapLoaded[n][m].pos.z - tollerance) && (updatedCarPos[carIndex].z <= mapLoaded[n][m].pos.z + tollerance);
				if (checkOnX && checkOnZ) {
					rightTurnsCrossed[carIndex][i] = true; 
					return true;

				}
			}
		}
		return false; 
	}

	bool carInTurnLeft(int carIndex, float deltaT) {
		int n;
		int m;
		float tollerance = 2.0f * carVelocity[carIndex] * deltaT;
		for (int i = 0; i < mapIndexes[LEFT].size(); i++) {
			if (!leftTurnsCrossed[carIndex][i]) {
				n = mapIndexes[LEFT][i].first;
				m = mapIndexes[LEFT][i].second;
				bool checkOnX = (updatedCarPos[carIndex].x >= mapLoaded[n][m].pos.x - tollerance) && (updatedCarPos[carIndex].x <= mapLoaded[n][m].pos.x + tollerance);
				bool checkOnZ = (updatedCarPos[carIndex].z >= mapLoaded[n][m].pos.z - tollerance) && (updatedCarPos[carIndex].z <= mapLoaded[n][m].pos.z + tollerance);
				if (checkOnX && checkOnZ) {
					leftTurnsCrossed[carIndex][i] = true;
					return true;
				}
			}
		}
		return false; 
	}

	bool carFinishedLap(int car) {
		int numTurnRight = std::count(rightTurnsCrossed[car].begin(), rightTurnsCrossed[car].end(), true);
		int numTurnLeft = std::count(leftTurnsCrossed[car].begin(), leftTurnsCrossed[car].end(), true);
		if (numTurnRight = mapIndexes[RIGHT].size() && numTurnLeft == mapIndexes[LEFT].size() && straightTurnCrossed[car] == mapIndexes[RIGHT].size()) {
			return true;
		}
		else return false; 
	}

	void cleanCarLapData(int car) {
		std::fill(rightTurnsCrossed[car].begin(), rightTurnsCrossed[car].end(), false);
		std::fill(leftTurnsCrossed[car].begin(), leftTurnsCrossed[car].end(), false);
		straightTurnCrossed[car] = 0; 
	}

	//Handles the camera movement and updates the view matrix
	void CameraPositionHandler(glm::vec3& r, float deltaT, glm::vec3& m, glm::mat4& vpMat, glm::mat4& pMat)
	{
		static glm::vec3 dampedCamPos = camPos;
		alpha += ROT_SPEED * r.y * deltaT;		// yaw, += for proper mouse movement
		beta -= ROT_SPEED * r.x * deltaT;		// pitch
		camDist -= MOVE_SPEED * deltaT * m.y;

		beta = (beta < 0.0f ? 0.0f : (beta > M_PI_2 - 0.4f ? M_PI_2 - 0.4f : beta));	// -0.3f to avoid camera flip for every camera distance
		camDist = (camDist < 5.0f ? 5.0f : (camDist > 15.0f ? 15.0f : camDist));	    // Camera distance limits

		camPos = updatedCarPos[player_car] + glm::vec3(-glm::rotate(glm::mat4(1), alpha + steeringAng[player_car] + glm::radians(initialRotation), glm::vec3(0, 1, 0)) * //update camera position based on car position
			glm::rotate(glm::mat4(1), beta, glm::vec3(1, 0, 0)) *
			glm::vec4(0, -camHeight, camDist, 1));
		dampedCamPos = camPos * (1 - exp(-lambdaCam * deltaT)) + dampedCamPos * exp(-lambdaCam * deltaT); //apply camera damping

		viewMatrix = glm::lookAt(dampedCamPos, updatedCarPos[player_car], upVector);
		vpMat = pMat * viewMatrix;
	}
};

// This is the main: probably you do not need to touch this!
int main() {
	CG_PRJ app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

