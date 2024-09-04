#include "modules/Starter.hpp"
#include <filesystem>
#include <map>
#include <string>
#include <random>
#include <audio.hpp>

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

struct CarLightsUniformBufferObject {
	// Headlights
	alignas(16) glm::vec3 headlightPosition[NUM_CARS][2];  //left and right
	alignas(16) glm::vec3 headlightDirection[NUM_CARS][2];
	alignas(16) glm::vec4 headlightColor[NUM_CARS][2];

	//Rear lights
	alignas(16) glm::vec3 rearLightPosition[NUM_CARS][2];  //left and right
	alignas(16) glm::vec3 rearLightDirection[NUM_CARS][2];
	alignas(16) glm::vec4 rearLightColor[NUM_CARS][2];
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

struct Checkpoint {
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

	//Cp
	Model Mcp;
	DescriptorSet DScp;

	//Road
	DescriptorSetLayout DSLroad;
	VertexDescriptor VD;
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
	Pipeline Pcar;
	std::vector<Model> Mcar;
	std::vector<DescriptorSet> DScar;

	// Environment
	DescriptorSetLayout DSLenvironment;
	Pipeline Penv;
	std::vector<Model> Menv;
	Texture Tenv;
	std::vector<DescriptorSet> DSenvironment;
	std::map<int, std::string> envFileNames;
	const std::string envModelsPath = "models/environment";
	std::vector<std::vector<std::pair <int, int>>> envIndexesPerModel;

	/******* APP PARAMETERS *******/
	float ar;
	const float FOVy = glm::radians(75.0f);
	const float nearPlane = 0.1f;
	const float farPlane = 500.0f;
	const float baseObjectRotation = 90.0f;
	Audio audio;

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
	std::vector<float> nextAng; 

	glm::vec3 oldUpdatedCarPos; // only for playerCar

	// Assume initialRotation is the rotation applied to the car model at spawn, represented as a quaternion
	glm::quat initialRotationQuat;
	std::map<int, glm::quat> steeringRotation; 
	std::map<int, glm::quat> totalRotation; 
	std::map<int, glm::vec3> forwardDir; 
	std::map<int, int> nextRightTurn; 
	std::map<int, int> nextLeftTurn; 
	std::map<int, int> car_laps; 
	std::map<int, bool> intermediateCheckpointIsCrossed; // for bot cars

	/******* MAP PARAMETERS *******/
	nlohmann::json mapFile;
	const int MAP_CENTER = MAP_SIZE / 2;
	int maxLaps = 2;
	std::vector<glm::vec3> roadsPosition; 
	std::vector<std::vector<RoadPosition>> mapLoaded;
	std::vector<std::vector<std::pair<int, int>>> mapIndexes; // 0: STRAIGHT, 1: LEFT, 2: RIGHT
	std::map<int, Checkpoint> checkpoints;
	glm::vec3 end_position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 center_road_position = glm::vec3(0.0f, 0.0f, 0.0f); 
	float checkpointOffset = 6.0f; 


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

	/******* RACE PARAMETERS *******/
	int currentCheckpoint;
	const int player_car = 0;
	bool raceIsEnded = false; 
	int winner; 
	float sensitivityValue = 8.0f; 

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
		if (w == 0 || h == 0) {
			// Window is minimized or has invalid dimensions, skip the update.
			return;
		}
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
		}
		else {
			throw std::invalid_argument("Invalid enum string: " + enumString);
		}
	}

	// Initialize everything needed for the application
	void localInit() {
		//Audio
		if (!audio.InitAudio()) {
			std::cerr << "Failed to initialize audio" << std::endl;
			exit(1);
		}
		if (!audio.LoadSounds()) {
			std::cerr << "Failed to load sounds" << std::endl;
			exit(1);
		}

		startingCarPos.resize(NUM_CARS);
		updatedCarPos.resize(NUM_CARS);
		carVelocity.resize(NUM_CARS);
		steeringAng.resize(NUM_CARS);
		nextAng.resize(NUM_CARS); 
		for (int i = 0; i < startingCarPos.size(); i++) {
			carVelocity[i] = 0.0f;
			steeringAng[i] = 0.0f;
		}
		currentCheckpoint = 0; 

		InitDSL();
		InitVD();
		InitPipelines();
		InitModels();

		//Map Grid Initialization
		mapFile = LoadMapFile();
		LoadMap(mapFile);

		//Environment models
		//readModels(envModelsPath);
		Menv.resize(envFileNames.size());
		for (const auto& [key, value] : envFileNames) {
			Menv[key].init(this, &VD, value, MGCG);
		}
		InitEnvironment();

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
			{ 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(CarLightsUniformBufferObject), 1 },
			{ 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(RoadLightsUniformBufferObject), 1 }
		});

		//Car
		DSLcar.init(this, {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(CarUniformBufferObject), 1 },
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1 }
		});

		//Environment
		DSLenvironment.init(this, {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(EnvironmentUniformBufferObject), 1 },
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1 }
		});
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

		VD.init(this, {
			{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
		}, {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION },
			{ 0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv), sizeof(glm::vec2), UV },
			{ 0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal), sizeof(glm::vec3), NORMAL },
		});
	}

	//Pipelines
	void InitPipelines()
	{
		PSkyBox.init(this, &VDSkyBox, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", { &DSLSkyBox });
		PSkyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, false);
		Proad.init(this, &VD, "shaders/RoadVert.spv", "shaders/RoadFrag.spv", { &DSLGlobal, &DSLroad });
		Proad.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
		Pcar.init(this, &VD, "shaders/CarVert.spv", "shaders/CarFrag.spv", { &DSLGlobal, &DSLcar });
		Penv.init(this, &VD, "shaders/EnvVert.spv", "shaders/EnvFrag.spv", { &DSLGlobal, &DSLenvironment });
		Penv.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
	}

	//Models
	void InitModels()
	{
		MSkyBox.init(this, &VDSkyBox, "models/SkyBoxCube.obj", OBJ);

		Mcar.resize(NUM_CARS);
		for (int i = 0; i < Mcar.size(); i++) {
			Mcar[i].init(this, &VD, "models/cars/car_" + std::to_string(i) + ".mgcg", MGCG);
		}

		MstraightRoad.init(this, &VD, "models/road/straight.mgcg", MGCG);
		MturnLeft.init(this, &VD, "models/road/turn.mgcg", MGCG);
		MturnRight.init(this, &VD, "models/road/turn.mgcg", MGCG);
		Mtile.init(this, &VD, "models/road/green_tile.mgcg", MGCG);
		Mcp.init(this, &VD, "models/checkpoint.mgcg", MGCG);
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
	void readModels(std::string path) {
		std::vector<std::string> directories;
		int id = 0;

		try {
			for (const auto& entry : std::filesystem::directory_iterator(path)) {
				if (entry.is_directory()) {
					directories.push_back(path + "/" + entry.path().filename().generic_string());
				}
			}
		}
		catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "Filesystem error: " << e.what() << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "General error: " << e.what() << std::endl;
		}

		for (const auto& dir : directories) {
			try {
				for (const auto& entry : std::filesystem::directory_iterator(dir)) {
					if (entry.is_regular_file()) {
						envFileNames[id++] = entry.path().generic_string();
					}
				}
			}
			catch (const std::filesystem::filesystem_error& e) {
				std::cerr << "Filesystem error: " << e.what() << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "General error: " << e.what() << std::endl;
			}
		}
	}

	//Loads the JSON
	nlohmann::json LoadMapFile() {
		nlohmann::json json;

		std::ifstream infile("config/map_hor.json");
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

		initialRotation = ((int)json["_map"][1]["col"] - previousItemIndex.second > 0) ? 270.0f : ((int)json["_map"][1]["col"] - previousItemIndex.second < 0) ? 90.0f : 0.0f; // Set the initial rotation 
		mapLoaded[previousItemIndex.first][previousItemIndex.second].rotation = initialRotation;
		initialRotationQuat = glm::quat(glm::vec3(0.0f, glm::radians(initialRotation), 0.0f)); //Represents the rotation applied to the car model at spawn

		int lastCpIndex = 0;
		for (const auto& [jsonKey, jsonValues] : json.items()) {
			if (jsonKey == "_map") {
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

					if (mapValues["type"] == "STRAIGHT" || mapValues["type"] == "LEFT" || mapValues["type"] == "RIGHT") {
						roadsPosition.push_back(mapLoaded[index.first][index.second].pos);
					}


				}
			}
			else if (jsonKey == "checkpoints") {
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
			else if (jsonKey == "start") {
				std::pair <int, int> startPosIndex = std::make_pair(jsonValues["row"], jsonValues["col"]);
				startingCarPos[player_car] = mapLoaded[startPosIndex.first][startPosIndex.second].pos;
				for (int i = 0; i < startingCarPos.size(); i++) {
					if (i != player_car) {
						startingCarPos[i] =
							startingCarPos[player_car] + glm::vec3(
								glm::rotate(glm::mat4(1.0f), glm::radians(initialRotation), glm::vec3(0.0f, 1.0f, 0.0f)) *
								glm::vec4(0.0f, 0.0f, -4.0f * i, 1.0f));
					}
				}
				camPos = glm::vec3(startingCarPos[player_car].x, camHeight, startingCarPos[player_car].z - camDist);
				updatedCarPos = startingCarPos;

			}
			else if (jsonKey == "end") {
				std::pair <int, int> endPosIndex;
				//int cpIndex;
				if (jsonValues == nullptr) {
					endPosIndex = std::make_pair(json["_map"].back()["row"], json["_map"].back()["col"]);
					std::cout << endPosIndex.first << endPosIndex.second << std::endl; 
				}
				else {
					endPosIndex = std::make_pair(jsonValues["row"], jsonValues["col"]);
					maxLaps = 1;
				}
				end_position = mapLoaded[endPosIndex.first][endPosIndex.second].pos;
				float rotation = mapLoaded[endPosIndex.first][endPosIndex.second].rotation;
				initCheckpoint(end_position, rotation, lastCpIndex);
				lastCpIndex++; 
			}

			for (int i = 0; i < NUM_CARS; i++) {
				car_laps[i] = 0;
				nextRightTurn[i] = 0;
				nextLeftTurn[i] = 0;
				intermediateCheckpointIsCrossed[i] = false;
			}

			int mid = (roadsPosition.size() - 1) / 2;
			center_road_position = roadsPosition[mid];
		}
	}

	void initCheckpoint(glm::vec3& checkpointPos, float rotation, int id)
	{
		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
		checkpoints[id].position = checkpointPos;
		checkpoints[id].pointA = checkpointPos + glm::vec3(rotationMatrix * glm::vec4(-checkpointOffset, 0.0f, checkpointOffset, 1.0f));
		checkpoints[id].pointB = checkpointPos + glm::vec3(rotationMatrix * glm::vec4(checkpointOffset, 0.0f, checkpointOffset, 1.0f));
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
		DPSZs.uniformBlocksInPool = 2 + 3 * 5 + Mcar.size() + Menv.size();			// summation of (#ubo * #DS) for each DSL	 (Gl SK + Road + Car + Env)
		DPSZs.texturesInPool = 2 + 5 + Mcar.size() + Menv.size();					// summation of (#texure * #DS) for each DSL (SK*2 + Road + Car + Env)
		DPSZs.setsInPool = 7 + Mcar.size() + Menv.size();							// summation of #DS for each DSL			 (Gl SK 5*Road + Car + Env)

		std::cout << "Uniform Blocks in the Pool  : " << DPSZs.uniformBlocksInPool << "\n";
		std::cout << "Textures in the Pool        : " << DPSZs.texturesInPool << "\n";
		std::cout << "Descriptor Sets in the Pool : " << DPSZs.setsInPool << "\n";
	}

	// Initialize pipelines and Descriptor Sets
	void pipelinesAndDescriptorSetsInit() {
		//Descriptor Set initialization
		switch (scene) {
		case 0:
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
		DScp.init(this, &DSLroad, { &Tenv });

		DScar.resize(NUM_CARS);
		for (int i = 0; i < DScar.size(); i++) {
			DScar[i].init(this, &DSLcar, { &Tenv });
		}

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

	// Destroys pipelines and Descriptor Sets
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
		DScp.cleanup();

		for (int i = 0; i < DScar.size(); i++) {
			DScar[i].cleanup();
		}
		for (int i = 0; i < DSenvironment.size(); i++) {
			DSenvironment[i].cleanup();
		}
	}

	// Destroys Models, Texture and Descr Set Layouts
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
		Mcp.cleanup();

		for (int i = 0; i < Mcar.size(); i++) {
			Mcar[i].cleanup();
		}
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

		//Audio Cleanup
		audio.AudioCleanup();
	}

	// Creates the command buffer:
	// Sends to the GPU all the objects to draw, with their buffers and textures
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

		//Draw Checkpoints
		Mcp.bind(commandBuffer);
		DScp.bind(commandBuffer, Proad, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(Mcp.indices.size()), static_cast<uint32_t>(checkpoints.size() * 2), 0, 0, 0);

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

	// Updates the uniform buffer
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
		
		if (!raceIsEnded) {
			CarsMotionHandler(deltaT, m);
			CheckpointHandler(deltaT, m);

		} else {
			steeringAng[winner] += 15.0f * carSteeringSpeed * deltaT; 
		}
		//Walk model procedure 
		CameraPositionHandler(r, deltaT, m, vpMat, pMat);

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

		//Global
		GlobalUniformBufferObject* g_ubo = new GlobalUniformBufferObject();
		if (scene != 3)
			g_ubo->lightDir = glm::vec3(0.0f, sin(glm::radians(180.0f) - rad_per_sec * turningTime), cos(glm::radians(180.0f) - rad_per_sec * turningTime));
		else
			g_ubo->lightDir = glm::vec3(0.0f, sin(glm::radians(180.0f) - rad_per_sec * (turningTime - sun_cycle_duration)), cos(glm::radians(180.0f) - rad_per_sec * (turningTime - sun_cycle_duration)));

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
		g_ubo->lightColor = glm::vec4(startingColor * (1 - timeFactor) + finalColor * timeFactor, 1.0f);
		g_ubo->viewerPosition = glm::vec3(glm::inverse(viewMatrix) * glm::vec4(0, 0, 0, 1)); // would dampedCam make sense?
		DSGlobal.map(currentImage, g_ubo, 0);

		//SkyBox
		skyBoxUniformBufferObject* sb_ubo = new skyBoxUniformBufferObject();
		sb_ubo->mvpMat = pMat * glm::mat4(glm::mat3(viewMatrix)); //Remove Translation part of ViewMatrix, take only Rotation part and applies Projection
		DSSkyBox.map(currentImage, sb_ubo, 0);

		//Player Car
		CarUniformBufferObject* car_ubo = new CarUniformBufferObject();
		for (int i = 0; i < NUM_CARS; i++) {
			car_ubo->mMat = glm::translate(glm::mat4(1.0f), updatedCarPos[i]) *
				glm::rotate(glm::mat4(1.0f), glm::radians(180.0f + initialRotation) + steeringAng[i], glm::vec3(0, 1, 0));
			car_ubo->mvpMat = vpMat * car_ubo->mMat;
			car_ubo->nMat = glm::inverse(glm::transpose(car_ubo->mMat));
			DScar[i].map(currentImage, car_ubo, 0);
		}
		
		CarLightsUniformBufferObject* carLights_ubo = new CarLightsUniformBufferObject();
		for (int j = 0; j < NUM_CARS; j++){
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), steeringAng[j], glm::vec3(0.0f, 1.0f, 0.0f));
			for (int i = 0; i < 2; i++) {
				glm::vec3 lightsOffset = glm::vec3((i == 0) ? -0.5f : 0.5f, 0.6f, -1.5f);
				carLights_ubo->headlightPosition[j][i] = updatedCarPos[j] + glm::vec3(rotationMatrix * glm::vec4(lightsOffset, 1.0f));
				carLights_ubo->headlightDirection[j][i] = glm::vec3(rotationMatrix * glm::vec4(0.0f, -0.2f, -1.0f, 0.0f)); //pointing forward
				if (scene == 3) {
					carLights_ubo->headlightColor[j][i] = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f); //white
				}
				else {
					carLights_ubo->headlightColor[j][i] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
				}
				lightsOffset = glm::vec3((i == 0) ? -0.55f : 0.55f, 0.6f, 1.9f);
				carLights_ubo->rearLightPosition[j][i] = updatedCarPos[j] + glm::vec3(rotationMatrix * glm::vec4(lightsOffset, 1.0f));
				carLights_ubo->rearLightDirection[j][i] = glm::vec3(rotationMatrix * glm::vec4(0.0f, -0.2f, 1.0f, 0.0f)); //pointing backwards
				if (scene == 3) {
					carLights_ubo->rearLightColor[j][i] = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f); //red
				}
				else {
					carLights_ubo->rearLightColor[j][i] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
				}
			}
		}

		//Road
		RoadUniformBufferObject* straight_road_ubo = new RoadUniformBufferObject();
		RoadLightsUniformBufferObject* lights_straight_road_ubo = new RoadLightsUniformBufferObject(); 
		for (int i = 0; i < mapIndexes[STRAIGHT].size(); i++) {
			int n = mapIndexes[STRAIGHT][i].first;
			int m = mapIndexes[STRAIGHT][i].second;
			straight_road_ubo->mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
										 glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation + baseObjectRotation), glm::vec3(0, 1, 0));
			straight_road_ubo->mvpMat[i] = vpMat * straight_road_ubo->mMat[i];
			straight_road_ubo->nMat[i] = glm::inverse(glm::transpose(straight_road_ubo->mMat[i]));
			
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
			lights_straight_road_ubo->spotLight_lightPosition[i][0] = transform * glm::vec4(-4.9f, 4.9f, -0.2f, 1.0f);
			lights_straight_road_ubo->spotLight_spotDirection[i][0] = rotation * glm::vec4(0.4f, -1.0f, 0.0f, 1.0f);
			
			if (!oneCondition) {
				lights_straight_road_ubo->spotLight_lightPosition[i][1] = transform * glm::vec4(4.9f, 4.9f, 7.8f, 1.0f);
				lights_straight_road_ubo->spotLight_spotDirection[i][1] = rotation * glm::vec4(-0.4f, -1.0f, 0.0f, 1.0f);
			}

			if (!m_oneCondition) {
				lights_straight_road_ubo->spotLight_lightPosition[i][2] = transform * glm::vec4(4.9f, 4.9f, -7.8f, 1.0f);
				lights_straight_road_ubo->spotLight_spotDirection[i][2] = rotation * glm::vec4(-0.4f, -1.0f, 0.0f, 1.0f);
			} 
		}
		if (scene == 3) {
			lights_straight_road_ubo->lightColorSpot = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f);
		}
		else {
			lights_straight_road_ubo->lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		DSstraightRoad.map(currentImage, straight_road_ubo, 1);
		DSstraightRoad.map(currentImage, carLights_ubo, 2);
		DSstraightRoad.map(currentImage, lights_straight_road_ubo, 3);

		//--------------------Turn Right
		RoadUniformBufferObject* turn_right = new RoadUniformBufferObject();
		RoadLightsUniformBufferObject* lights_turn_right_road_ubo = new RoadLightsUniformBufferObject();
		for (int i = 0; i < mapIndexes[RIGHT].size(); i++) {
			int n = mapIndexes[RIGHT][i].first;
			int m = mapIndexes[RIGHT][i].second;
			turn_right->mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
								 glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation + baseObjectRotation), glm::vec3(0, 1, 0));
			turn_right->mvpMat[i] = vpMat * turn_right->mMat[i];
			turn_right->nMat[i] = glm::inverse(glm::transpose(turn_right->mMat[i]));		
			
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation), glm::vec3(0, 1, 0));
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) * rotation;

			// Spot positions
			lights_turn_right_road_ubo->spotLight_lightPosition[i][0] = transform * glm::vec4(-4.85f, 4.9f, -5.9f, 1.0f);

			// Spot directions
			lights_turn_right_road_ubo->spotLight_spotDirection[i][0] = rotation * glm::vec4(0.4f, -1.0f, 0.4f, 1.0f);
		}

		if (scene == 3) {
			lights_turn_right_road_ubo->lightColorSpot = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f);
		}
		else {
			lights_turn_right_road_ubo->lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		DSturnRight.map(currentImage, turn_right, 1);
		DSturnRight.map(currentImage, carLights_ubo, 2);
		DSturnRight.map(currentImage, lights_turn_right_road_ubo, 3);

		//--------------------Turn Left
		RoadUniformBufferObject* turn_left = new RoadUniformBufferObject();
		RoadLightsUniformBufferObject* lights_turn_left_road_ubo = new RoadLightsUniformBufferObject();
		for (int i = 0; i < mapIndexes[LEFT].size(); i++) {
			int n = mapIndexes[LEFT][i].first;
			int m = mapIndexes[LEFT][i].second;
			turn_left->mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) *
								glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation + baseObjectRotation), glm::vec3(0, 1, 0));
			turn_left->mvpMat[i] = vpMat * turn_left->mMat[i];
			turn_left->nMat[i] = glm::inverse(glm::transpose(turn_left->mMat[i]));

			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(mapLoaded[n][m].rotation - 90.0f), glm::vec3(0, 1, 0));
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos) * rotation;

			// Spot positions
			lights_turn_left_road_ubo->spotLight_lightPosition[i][0] = transform * glm::vec4(-5.8f, 5.0f, 4.65f, 1.0f);

			// Spot directions
			lights_turn_left_road_ubo->spotLight_spotDirection[i][0] = rotation * glm::vec4(0.4f, -1.0f, -0.4f, 1.0f);
		}

		if (scene == 3) {
			lights_turn_left_road_ubo->lightColorSpot = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f);
		}
		else{
			lights_turn_left_road_ubo->lightColorSpot = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
		DSturnLeft.map(currentImage, turn_left, 1);
		DSturnLeft.map(currentImage, carLights_ubo, 2);
		DSturnLeft.map(currentImage, lights_turn_left_road_ubo, 3);

		//--------------------Tile
		RoadUniformBufferObject* r_tile = new RoadUniformBufferObject();
		RoadLightsUniformBufferObject* lights_tile_ubo = new RoadLightsUniformBufferObject();
		for (int i = 0; i < mapIndexes[NONE].size(); i++) {
			int n = mapIndexes[NONE][i].first;
			int m = mapIndexes[NONE][i].second;
			r_tile->mMat[i] = glm::translate(glm::mat4(1.0f), mapLoaded[n][m].pos);
			r_tile->mvpMat[i] = vpMat * r_tile->mMat[i];
			r_tile->nMat[i] = glm::inverse(glm::transpose(r_tile->mMat[i]));
			lights_tile_ubo->spotLight_lightPosition[i][0] = glm::vec4(0.0f);  
			lights_tile_ubo->spotLight_spotDirection[i][0] = glm::vec4(0.0f);
			lights_tile_ubo->spotLight_lightPosition[i][1] = glm::vec4(0.0f);
			lights_tile_ubo->spotLight_spotDirection[i][1] = glm::vec4(0.0f);
			lights_tile_ubo->spotLight_lightPosition[i][2] = glm::vec4(0.0f);
			lights_tile_ubo->spotLight_spotDirection[i][2] = glm::vec4(0.0f);
		}
		lights_tile_ubo->lightColorSpot = glm::vec4(0.0f);
		lights_tile_ubo->lightColorSpot = glm::vec4(0.0f);
		DStile.map(currentImage, r_tile, 1);
		DStile.map(currentImage, carLights_ubo, 2);
		DStile.map(currentImage, lights_tile_ubo, 3);

		// Checkpoints
		RoadUniformBufferObject* cp_ubo = new RoadUniformBufferObject();
		for (int i = 0, j = 0; i < checkpoints.size() * 2; i+=2, j++) {
			cp_ubo->mMat[i] = glm::translate(glm::mat4(1.0f), checkpoints[j].pointA);
			cp_ubo->mvpMat[i] = vpMat * cp_ubo->mMat[i];
			cp_ubo->nMat[i] = glm::inverse(glm::transpose(cp_ubo->mMat[i]));

			cp_ubo->mMat[i + 1] = glm::translate(glm::mat4(1.0f), checkpoints[j].pointB);
			cp_ubo->mvpMat[i + 1] = vpMat * cp_ubo->mMat[i + 1];
			cp_ubo->nMat[i + 1] = glm::inverse(glm::transpose(cp_ubo->mMat[i + 1]));
		}
		DScp.map(currentImage, cp_ubo, 1);
		DScp.map(currentImage, carLights_ubo, 2);
		DScp.map(currentImage, lights_tile_ubo, 3);

		//Environment
		EnvironmentUniformBufferObject env_ubo{};
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
		}
	}

	// Handles checkpoint updates Checkpoint
	void CheckpointHandler(float deltaT, glm::vec3 m) {
		if (IsBetweenPoints(updatedCarPos[player_car], checkpoints[currentCheckpoint], deltaT, m)) {
			currentCheckpoint++;
			std::cout << "Lap: " << car_laps[player_car] << " Checkpoint: " << currentCheckpoint << std::endl;
			if (currentCheckpoint == checkpoints.size()) {
				currentCheckpoint = 0;
				car_laps[player_car] += 1;
				audio.PlayLapSound();
			}
			else {
				audio.PlayCheckpointSound();
			}
		}
		
		for (int i = 0; i < NUM_CARS; i++) {
			if (i != player_car) {
				if (intermediateCheckpointIsCrossed[i]) {
					lapUpdatingHandler(i, deltaT);
				}
				else {
					if (checkDistance(center_road_position, i, deltaT)) {
						intermediateCheckpointIsCrossed[i] = true;
					}
				}
			}
			winnerHandler(i); 
		}
	}
	
	// Function to handle laps
	void lapUpdatingHandler(int carIndex, float deltaT) {
		if (checkDistance(checkpoints[checkpoints.size() - 1].position, carIndex, deltaT)) {
			car_laps[carIndex]++;
			intermediateCheckpointIsCrossed[carIndex] = false;
		}
	}

	// Function to check if the car has won
	void winnerHandler(int carIndex) {
		if (car_laps[carIndex] == maxLaps) {
			raceIsEnded = true;
			winner = carIndex;
			updatedCarPos[carIndex] = end_position;
			audio.PlayClappingSound();
		}
	}

	// Function to check if the car is crossing the checkpoint
	bool IsBetweenPoints(const glm::vec3& carPos, const Checkpoint& checkpoint, float deltaT, glm::vec3 m) {
		
		bool isInRoad = false;
		bool checkpointIsCrossed = false;
		float epsilon = 0.0001f; 
		float outOfRoadTollerance = 1.5f; 
		float zToCross = checkpoint.pointA.z;
		float xToCross = checkpoint.pointA.x; 

		if (abs(checkpoint.pointA.z - checkpoint.pointB.z) < epsilon) { // MOVING ALONG Z
			if (zToCross - oldUpdatedCarPos.z > 0.0f) { //POSITIVE Z AXES
				checkpointIsCrossed = carPos.z >= zToCross;
			}
			else if (zToCross - oldUpdatedCarPos.z < 0.0f) { // NEGATIVE Z AXES
				checkpointIsCrossed = carPos.z <= zToCross;
			}
			isInRoad = carPos.x <= glm::max(checkpoint.pointA.x, checkpoint.pointB.x) + outOfRoadTollerance && carPos.x >= glm::min(checkpoint.pointA.x, checkpoint.pointB.x) - outOfRoadTollerance;
		}

		else if (abs(checkpoint.pointA.x - checkpoint.pointB.x) < epsilon) { // MOVING ALONG X
			if (xToCross - oldUpdatedCarPos.x > 0.0f) { // POSITIVE X AXES
				checkpointIsCrossed = carPos.x >= xToCross;
			}
			else if (xToCross - oldUpdatedCarPos.x < 0.0f) { // NEGATIVE X AXES
				checkpointIsCrossed = carPos.x <= xToCross;
			}
			isInRoad = carPos.z <= glm::max(checkpoint.pointA.z, checkpoint.pointB.z) + outOfRoadTollerance && carPos.z >= glm::min(checkpoint.pointA.z, checkpoint.pointB.z) - outOfRoadTollerance;
		}
		return isInRoad && checkpointIsCrossed;
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
		steeringRotation[player_car] = glm::angleAxis(steeringAng[player_car], glm::vec3(0.0f, 1.0f, 0.0f));
		totalRotation[player_car] = initialRotationQuat * steeringRotation[player_car];

		forwardDir[player_car] = totalRotation[player_car] * glm::vec3(0.0f, 0.0f, -1.0f);	//the forward direction in global space
		startingCarPos[player_car] += forwardDir[player_car] * carVelocity[player_car] * deltaT;
		oldUpdatedCarPos = updatedCarPos[player_car];
		updatedCarPos[player_car] = updatedCarPos[player_car] * std::exp(-carDamping * deltaT) + startingCarPos[player_car] * (1 - std::exp(-carDamping * deltaT));

		CollisionHandler(forwardDir[player_car], deltaT);
		for (int i = 0; i < NUM_CARS; i++) {
			if (i != player_car) {
				if (nextAng[i] == 0.0f) {
					manageCarDirection(i, deltaT);
				}
				else {
					steeringAng[i] += nextAng[i];
					nextAng[i] = 0.0f;
				}
				updatedCarPos[i] = updatedCarPos[i] * std::exp(-carDamping * deltaT) + startingCarPos[i] * (1 - std::exp(-carDamping * deltaT));
			}
		}
	}

	// Collision detection and response
	void CollisionHandler(glm::vec3& forwardDir, float deltaT)
	{
		float minBoundary = -SCALING_FACTOR * MAP_CENTER;
		float maxBoundary = SCALING_FACTOR * MAP_CENTER ;
		
		glm::vec3 predictedPos = updatedCarPos[player_car] + forwardDir * carVelocity[player_car] * deltaT;
		bool collided = false;

		if (predictedPos.x < minBoundary) {
			predictedPos.x = minBoundary;
			collided = true;
		}
		else if (predictedPos.x > maxBoundary) {
			predictedPos.x = maxBoundary;
			collided = true;
		}

		if (predictedPos.z < minBoundary) {
			predictedPos.z = minBoundary;
			collided = true;
		}
		else if (predictedPos.z > maxBoundary) {
			predictedPos.z = maxBoundary;
			collided = true;
		}
		
		if (collided) {
			carVelocity[player_car] *= -0.5f; 
		}

		// Apply the position update after collision response
		updatedCarPos[player_car] = predictedPos;
	}

	// Manages the car direction and updates the car position
	void manageCarDirection(int carIndex, float deltaT) {
		int n; 
		int m;
		if(mapIndexes[LEFT].size() != 0){
			n = mapIndexes[LEFT][nextLeftTurn[carIndex]].first; 
			m = mapIndexes[LEFT][nextLeftTurn[carIndex]].second;
			if (checkDistance(mapLoaded[n][m].pos, carIndex, deltaT)) {
				nextAng[carIndex] = glm::radians(90.0f);
				nextLeftTurn[carIndex] = (nextLeftTurn[carIndex] + 1) % mapIndexes[LEFT].size(); 
				startingCarPos[carIndex] = mapLoaded[n][m].pos;
				return;
			}
		}

		if(mapIndexes[RIGHT].size() != 0){
			n = mapIndexes[RIGHT][nextRightTurn[carIndex]].first; 
			m = mapIndexes[RIGHT][nextRightTurn[carIndex]].second;
			if (checkDistance(mapLoaded[n][m].pos, carIndex, deltaT)) {
				nextAng[carIndex] = -glm::radians(90.0f);
				nextRightTurn[carIndex] = (nextRightTurn[carIndex] + 1) % mapIndexes[RIGHT].size();
				startingCarPos[carIndex] = mapLoaded[n][m].pos;
				return;
			}
		}
		carVelocity[carIndex] += carAcceleration * deltaT;
		carVelocity[carIndex] = glm::min(carVelocity[carIndex], 70.0f - (25.0f * carIndex));

		steeringRotation[carIndex] = glm::angleAxis(steeringAng[carIndex], glm::vec3(0.0f, 1.0f, 0.0f));
		totalRotation[carIndex] = initialRotationQuat * steeringRotation[carIndex];
		forwardDir[carIndex] = totalRotation[carIndex] * glm::vec3(0.0f, 0.0f, -1.0f);
		startingCarPos[carIndex] += forwardDir[carIndex] * carVelocity[carIndex] * deltaT;
		return; 
	}

	// Checks if the car is close to the target position
	bool checkDistance(glm::vec3 targetPos, int carIndex, float deltaT) {
		float potVelocity = carVelocity[carIndex] + (2 * carAcceleration * deltaT); 
		bool checkOnX = abs(updatedCarPos[carIndex].x - targetPos.x) <= abs(forwardDir[carIndex].x * potVelocity * deltaT) + abs(potVelocity * deltaT);
		bool checkOnZ = abs(updatedCarPos[carIndex].z - targetPos.z) <= abs(forwardDir[carIndex].z * potVelocity * deltaT) + abs(potVelocity * deltaT);
		return (checkOnX && checkOnZ);
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

		if (!raceIsEnded) {
			camPos = updatedCarPos[player_car] + glm::vec3(-glm::rotate(glm::mat4(1), alpha + steeringAng[player_car] + glm::radians(initialRotation), glm::vec3(0, 1, 0)) * //update camera position based on car position
				glm::rotate(glm::mat4(1), beta, glm::vec3(1, 0, 0)) *
				glm::vec4(0, -camHeight, camDist, 1));
		}
		else {
			float offset = 40.0f;

			if (end_position.x > 0 && end_position.z > 0) {
				camPos = glm::vec3(end_position.x + offset, offset - 15.0f, end_position.z + offset);
			}
			if (end_position.x > 0 && end_position.z < 0) {
				camPos = glm::vec3(end_position.x + offset, offset - 15.0f, end_position.z - offset);
			}
			if (end_position.x < 0 && end_position.z > 0) {
				camPos = glm::vec3(end_position.x - offset, offset - 15.0f, end_position.z + offset);
			}
			if (end_position.x < 0 && end_position.z < 0) {
				camPos = glm::vec3(end_position.x - offset, offset - 15.0f, end_position.z - offset);
			}
		}
		dampedCamPos = camPos * (1 - exp(-lambdaCam * deltaT)) + dampedCamPos * exp(-lambdaCam * deltaT); //apply camera damping

		if (!raceIsEnded) {
			viewMatrix = glm::lookAt(dampedCamPos, updatedCarPos[player_car], upVector);
		}
		else {
			viewMatrix = glm::lookAt(dampedCamPos, -end_position, upVector);
		}
		vpMat = pMat * viewMatrix;
	}
};

// This is the main: probably you do not need to touch this
int main(int argc, char *argv[]) {
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