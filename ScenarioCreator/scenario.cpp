#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_TR2_SYS_NAMESPACE_DEPRECATION_WARNING

#include "scenario.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <direct.h>

namespace fs = std::experimental::filesystem;

using namespace std::tr2::sys;

int logLenght = 100;
char *logString = (char*)malloc(logLenght * sizeof(char));

FILE *f;
//const char *filesPath = "JTA-Scenarios\\";
char fileName[20] = "None";
int nFiles = 0, currentFile = 1;

std::string statusText;
DWORD statusTextDrawTicksMax;
bool statusTextGxtEntry;

bool firstTime = true;
bool menuActive = false;
bool fly = false;

//menu commands
bool	bUp = false,
bDown = false,
bLeft = false,
bRight = false,
bEnter = false,
bBack = false,
bQuit = false;
bool stopCoords = false;

Vector3 playerCoords;
Vector3 fixCoords;
Vector3 goFrom, goTo;
Vector3 A, B, C;
Vector3 TP1, TP2, TP1_rot, TP2_rot;

//cam variables
Cam activeCam;
Vector3 camCoords, camRot;
bool camLock = true;
bool camMoving = false;

// variables concernning weather and time conditions

//menu parameters
float mHeight = 9, mTopFlat = 60, mTop = 36, mLeft = 3, mTitle = 5;
int activeLineIndexMain = 0;
bool visible = false;

//menu peds parameters
int activeLineIndexPeds = 0;
int nPeds = 1;
int currentBehaviour = 0;
bool group = false;
// The maximum number of pedestrians to spawn at one spot
//const int nMaxPeds = 1024;
const int numberTaks = 9;

// weather parameters
int weather=-1;



LPCSTR tasks[numberTaks] = {
	"SCENARIO",
	"STAND",
	"PHONE",
	"COWER",
	"WANDER",
	"CHAT",
	"COMBAT",
	"COVER",
	"MOVE"
};

bool subMenuActive = false;

//behaviour params
int maxNumber = 1000000;
const int nScenario = 14;
float speed = 1;

struct {
	LPCSTR  text;
	int param;
} paramsLines[10] = {
	{ "Task Time", 1000000 },
	{ "Type", 0 },
	{ "Radius", 20 },
	{ "Minimal Lenght", 1 },
	{ "Time between Walks", 1 },
	{ "Spawning Radius", 1 }
};

static char scenarioTypes[nScenario][40]{
	"NEAREST",
	"RANDOM",
	"WORLD_HUMAN_MUSICIAN",
	"WORLD_HUMAN_SMOKING",
	"WORLD_HUMAN_BINOCULARS",
	"WORLD_HUMAN_CHEERING",
	"WORLD_HUMAN_DRINKING",
	"WORLD_HUMAN_PARTYING",
	"WORLD_HUMAN_PICNIC",
	"WORLD_HUMAN_STUPOR",
	"WORLD_HUMAN_PUSH_UPS",
	"WORLD_HUMAN_LEANING",
	"WORLD_HUMAN_MUSCLE_FLEX",
	"WORLD_HUMAN_YOGA"
};

//wped
typedef struct wPed {
	Ped ped;
	bool goingTo = true;
	Vector3 from, to;
	int stopTime;
	int timeFix = -1;
	float speed;
} wPed;

wPed wPeds[200];
int nwPeds = 0;

int activeLineIndexCamera = 0;
int activeLineIndexPlace = 0;
int activeLineIndexTime = 0;
int activeLineIndexWeather = 0;
int activeLineIndexSubmenu = 0;
int activeLineIndexFile = 0;

bool wind = false;

inline void update_status_text()
{
	if (GetTickCount() < statusTextDrawTicksMax)
	{
		UI::SET_TEXT_FONT(0);
		UI::SET_TEXT_SCALE(0.55f, 0.55f);
		UI::SET_TEXT_COLOUR(255, 255, 255, 255);
		UI::SET_TEXT_WRAP(0.0, 1.0);
		UI::SET_TEXT_CENTRE(1);
		UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
		UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
		if (statusTextGxtEntry)
		{
			UI::_SET_TEXT_ENTRY((char *)statusText.c_str());
		}
		else
		{
			UI::_SET_TEXT_ENTRY((char*)"STRING");
			UI::_ADD_TEXT_COMPONENT_STRING((char *)statusText.c_str());
		}
		UI::_DRAW_TEXT(0.5f, 0.1f);
	}
}

void set_status_text(std::string str, DWORD time = 2000, bool isGxtEntry = false)
{
	statusText = str;
	statusTextDrawTicksMax = GetTickCount() + time;
	statusTextGxtEntry = isGxtEntry;
}

inline void firstOpen(const std::string& files_path)
{
	Player mainPlayer = PLAYER::PLAYER_ID();
	PLAYER::SET_PLAYER_INVINCIBLE(mainPlayer, TRUE);
	PLAYER::SET_POLICE_IGNORE_PLAYER(mainPlayer, TRUE);
	PLAYER::SET_EVERYONE_IGNORE_PLAYER(mainPlayer, TRUE);
	PLAYER::SPECIAL_ABILITY_FILL_METER(mainPlayer, 1);
	PLAYER::SET_PLAYER_NOISE_MULTIPLIER(mainPlayer, 0.0);
	PLAYER::SET_SWIM_MULTIPLIER_FOR_PLAYER(mainPlayer, 1.49f);
	PLAYER::SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER(mainPlayer, 1.49f);
	PLAYER::DISABLE_PLAYER_FIRING(mainPlayer, TRUE);
	PLAYER::SET_DISABLE_AMBIENT_MELEE_MOVE(mainPlayer, TRUE);
	goFrom = playerCoords;
	goTo = playerCoords;
	A = playerCoords;
	B = playerCoords;
	C = playerCoords;
	TP1 = playerCoords;
	TP2 = playerCoords;
	goTo.x += 2; goTo.y += 2;
	srand((unsigned int)time(NULL));
	strcpy(logString, "");
	_mkdir(files_path.c_str());
	firstTime = false;
}

inline void draw_rect(float A_0, float A_1, float A_2, float A_3, int A_4, int A_5, int A_6, int A_7)
{
	GRAPHICS::DRAW_RECT((A_0 + (A_2 * 0.5f)), (A_1 + (A_3 * 0.5f)), A_2, A_3, A_4, A_5, A_6, A_7);
}

inline void draw_menu_line(std::string caption, float lineWidth, float lineHeight, float lineTop, float lineLeft, float textLeft, bool active, bool title, bool rescaleText = true)
{
	// default values
	int text_col[4] = { 255, 255, 255, 255 },
		rect_col[4] = { 0, 0, 0, 190 };
	float text_scale = 0.35f;
	int font = 0;

	// correcting values for active line
	if (active)
	{
		text_col[0] = 0;
		text_col[1] = 0;
		text_col[2] = 0;

		rect_col[0] = 0;
		rect_col[1] = 180;
		rect_col[2] = 205;
		rect_col[3] = 220;

		if (rescaleText) text_scale = 0.35f;
	}

	if (title)
	{
		rect_col[0] = 0;
		rect_col[1] = 0;
		rect_col[2] = 0;

		if (rescaleText) text_scale = 0.70f;
		font = 1;
	}

	int screen_w, screen_h;
	GRAPHICS::GET_SCREEN_RESOLUTION(&screen_w, &screen_h);

	textLeft += lineLeft;

	float lineWidthScaled = lineWidth / (float)screen_w; // line width
	float lineTopScaled = lineTop / (float)screen_h; // line top offset
	float textLeftScaled = textLeft / (float)screen_w; // text left offset
	float lineHeightScaled = lineHeight / (float)screen_h; // line height

	float lineLeftScaled = lineLeft / (float)screen_w;

	// this is how it's done in original scripts

	// text upper part
	UI::SET_TEXT_FONT(font);
	UI::SET_TEXT_SCALE(0.0, text_scale);
	UI::SET_TEXT_COLOUR(text_col[0], text_col[1], text_col[2], text_col[3]);
	UI::SET_TEXT_CENTRE(0);
	UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
	UI::SET_TEXT_EDGE(0, 0, 0, 0, 0);
	UI::_SET_TEXT_ENTRY((char*)"STRING");
	UI::_ADD_TEXT_COMPONENT_STRING((LPSTR)caption.c_str());
	UI::_DRAW_TEXT(textLeftScaled, (((lineTopScaled + 0.00278f) + lineHeightScaled) - 0.005f));

	// text lower part
	UI::SET_TEXT_FONT(font);
	UI::SET_TEXT_SCALE(0.0, text_scale);
	UI::SET_TEXT_COLOUR(text_col[0], text_col[1], text_col[2], text_col[3]);
	UI::SET_TEXT_CENTRE(0);
	UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
	UI::SET_TEXT_EDGE(0, 0, 0, 0, 0);
	UI::_SET_TEXT_GXT_ENTRY((char*)"STRING");
	UI::_ADD_TEXT_COMPONENT_STRING((LPSTR)caption.c_str());
	int num25 = UI::_0x9040DFB09BE75706(textLeftScaled, (((lineTopScaled + 0.00278f) + lineHeightScaled) - 0.005f));

	// rect
	draw_rect(lineLeftScaled, lineTopScaled + (0.00278f),
		lineWidthScaled, ((((float)(num25)* UI::_0xDB88A37483346780(text_scale, 0)) + (lineHeightScaled * 2.0f)) + 0.005f),
		rect_col[0], rect_col[1], rect_col[2], rect_col[3]);
}

Cam lockCam(Vector3 pos, Vector3 rot) {
	CAM::DESTROY_ALL_CAMS(true);
	Cam lockedCam = CAM::CREATE_CAM_WITH_PARAMS((char*)"DEFAULT_SCRIPTED_CAMERA", pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, 50, true, 2);
	CAM::SET_CAM_ACTIVE(lockedCam, true);
	CAM::RENDER_SCRIPT_CAMS(1, 0, 3000, 1, 0);
	return lockedCam;
}

inline void camLockChange()
{
	if (camLock) {
		CAM::RENDER_SCRIPT_CAMS(0, 0, 3000, 1, 0);
	}
	else {
		Vector3 oldCamCoords = CAM::GET_GAMEPLAY_CAM_COORD();
		Vector3 oldCamRots = CAM::GET_GAMEPLAY_CAM_ROT(2);
		float fov = CAM::GET_GAMEPLAY_CAM_FOV();
		CAM::DESTROY_ALL_CAMS(true);
		activeCam = CAM::CREATE_CAM_WITH_PARAMS((char*)"DEFAULT_SCRIPTED_CAMERA", oldCamCoords.x, oldCamCoords.y, oldCamCoords.z, oldCamRots.x, oldCamRots.y, oldCamRots.z, fov, true, 2);
		CAM::SET_CAM_ACTIVE(activeCam, true);
		CAM::RENDER_SCRIPT_CAMS(1, 0, 3000, 1, 0);
	}
}


inline int saveFile(const std::string& files_path, const int def_weather, std::vector<Ped>& overall_peds)
{	
	const int line_count = 13;
	LPCSTR weather_lines[line_count] = {
		"EXTRASUNNY",
		"CLEAR",
		"CLOUDS",
		"SMOG",
		"FOGGY",
		"OVERCAST",
		"RAIN",
		"THUNDER",
		"CLEARING",
		"NEUTRAL",
		"SNOW",
		"BLIZZARD",
		"SNOWLIGHT",
	};

	std::string fname = "";
	int stop = 0;

	if (stopCoords)
		stop = 1;

	fname = fname + std::string(files_path) + std::string(fileName);
	
	f = fopen(fname.c_str(), "w");

	// camera parameters and whether moving or not
	if (camMoving)
		fprintf_s(f, "%d %f %f %f %d %f %f %f %f %f %f\n", (int)camMoving, A.x, A.y, A.z, stop, B.x, B.y, B.z, C.x, C.y, C.z);
	else
		fprintf_s(f, "%d %f %f %f %d %f %f %f\n", (int)camMoving, camCoords.x, camCoords.y, camCoords.z, stop, camRot.x, camRot.y, camRot.z);

	fprintf_s(f, "%f %f %f %f %f %f\n", TP1.x, TP1.y, TP1.z, TP1_rot.x, TP1_rot.y, TP1_rot.z);
	fprintf_s(f, "%f %f %f %f %f %f\n", TP2.x, TP2.y, TP2.z, TP2_rot.x, TP2_rot.y, TP2_rot.z);

	// store actual weather and wind 
	std::string weather_info = "Weather to be stored is " + weather;
	set_status_text(weather_info);
	if (weather == -1) {
		GAMEPLAY::CLEAR_OVERRIDE_WEATHER();
		GAMEPLAY::SET_WEATHER_TYPE_NOW_PERSIST((char *)weather_lines[def_weather]);
		GAMEPLAY::CLEAR_WEATHER_TYPE_PERSIST();
		weather = def_weather;
	}
	fprintf_s(f, "%d %d\n", static_cast<int>(wind), weather);
	
	//also write current time
	const auto hours = TIME::GET_CLOCK_HOURS();
	const auto minutes = TIME::GET_CLOCK_MINUTES();
	const auto secs = TIME::GET_CLOCK_SECONDS();
	fprintf_s(f, "%d %d %d\n", hours, minutes, secs);

	// logString saves the information about pedestrians
	fprintf_s(f, "%s", logString);
	fclose(f);

	// reset logString
	strcpy(logString, "");

	fname = fname + "  SAVED!";
	set_status_text(fname);

	for (auto& p : overall_peds) {
		PED::DELETE_PED(&p);
		WAIT(10);
	}

	return MAX_NUMBER_OVERALL_PEDS;
}

inline int readLine(FILE *f, Vector3 *pos)
{
	int ngroup = 0;
	int result = fscanf_s(f, "%d %f %f %f %d %d %f %f %f %f %f %f %f %d %d %d %d %d %d \n", &nPeds, &(*pos).x, &(*pos).y, &(*pos).z, &ngroup, &currentBehaviour, &speed, &goFrom.x, &goFrom.y, &goFrom.z, &goTo.x, &goTo.y, &goTo.z, &paramsLines[0].param, &paramsLines[1].param, &paramsLines[2].param, &paramsLines[3].param, &paramsLines[4].param, &paramsLines[5].param);
	if (ngroup == 0)
		group = false;
	if (ngroup == 1)
		group = 1;
	char message[5];
	sprintf_s(message, "%d", result);
	set_status_text(message);
	return result;
}

inline bool file_exist(std::string fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

inline void readDirr(const std::string& files_path) {
	int i = 0;
	std::string s;
	do {
		i++;
		s = files_path + "log_" + std::to_string(i) + ".txt";
		
	} while (file_exist(s));
	nFiles = i - 1;
}

inline void addwPed(Ped p, Vector3 from, Vector3 to, int stop, float spd)
{
	if (nwPeds > 199)
		return;

	wPeds[nwPeds].ped = p;
	wPeds[nwPeds].from = from;
	wPeds[nwPeds].to = to;
	wPeds[nwPeds].stopTime = stop;
	wPeds[nwPeds].speed = spd;

	nwPeds++;
}

Vector3 coordsToVector(float x, float y, float z)
{
	Vector3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

void writeLogLine(float x, float y, float z)
{
	char line[200];
	int size;
	//line format: "nPeds X Y Z group currentBehaviour speed goFrom[3] goTo[3] paramLines.param[5] \n"
	int ngroup = 0;
	if (group) ngroup = 1;
	sprintf_s(line, "%d %f %f %f %d %d %f %f %f %f %f %f %f %d %d %d %d %d %d \n", nPeds, x, y, z, ngroup, currentBehaviour, speed, goFrom.x, goFrom.y, goFrom.z, goTo.x, goTo.y, goTo.z, paramsLines[0].param, paramsLines[1].param, paramsLines[2].param, paramsLines[3].param, paramsLines[4].param, paramsLines[5].param);

	size = (int)(strlen(line) + strlen(logString));
	if (size >= logLenght)
		logString = (char*)realloc(logString, (100 + size) * sizeof(char));

	strcat(logString, line);
}



ScenarioCreator::ScenarioCreator(const std::string& parameters_file):string_params_(parameters_file),int_params_(parameters_file),nr_peds_left_(MAX_NUMBER_OVERALL_PEDS) {
	PLAYER::SET_EVERYONE_IGNORE_PLAYER(PLAYER::PLAYER_PED_ID(), TRUE);
	PLAYER::SET_POLICE_IGNORE_PLAYER(PLAYER::PLAYER_PED_ID(), TRUE);
	PLAYER::CLEAR_PLAYER_WANTED_LEVEL(PLAYER::PLAYER_PED_ID());
	ENTITY::SET_ENTITY_COLLISION(PLAYER::PLAYER_PED_ID(), TRUE, TRUE);
	ENTITY::SET_ENTITY_VISIBLE(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
	ENTITY::SET_ENTITY_ALPHA(PLAYER::PLAYER_PED_ID(), 255, FALSE);
	ENTITY::SET_ENTITY_CAN_BE_DAMAGED(PLAYER::PLAYER_PED_ID(), FALSE);

	registerParams();

	this->files_path_ = string_params_.getParam(this->scenario_file_param_name_) + "\\";
	this->default_weather_ = int_params_.getParam(this->def_weather_param_name_);
	this->max_nr_peds_per_spawn = int_params_.getParam(this->max_spawn_peds_param_name_);

	GAMEPLAY::SET_TIME_SCALE(1.0);

	//log_file.open("log.txt");
}

ScenarioCreator::~ScenarioCreator() {
	// todo: implement a destroyer
	ReleaseDC(hWnd, hWindowDC);
	DeleteDC(hCaptureDC);
	DeleteObject(hCaptureBitmap);
	//log_file.close();
}

void ScenarioCreator::update() {

	PLAYER::SET_POLICE_IGNORE_PLAYER(PLAYER::PLAYER_PED_ID(), TRUE);
	PLAYER::SET_EVERYONE_IGNORE_PLAYER(PLAYER::PLAYER_PED_ID(), TRUE);

	listen_for_keystrokes();

	cameraCoords();

	stopControl();

	walking_peds();

	update_status_text();
}

void ScenarioCreator::stopControl()
{
	if (stopCoords) {
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(PLAYER::PLAYER_PED_ID(), fixCoords.x, fixCoords.y, fixCoords.z, 0, 0, 0);
	}
}

void ScenarioCreator::listen_for_keystrokes() {

	//cancel last inseriment
	if (IsKeyJustUp(VK_F9)) {
		cancelLastLog();
		set_status_text("last log: CANCELLED!");
	}

	//clear log string
	if (IsKeyJustUp(VK_F11)) {
		logString[0] = '\0';
	}

	//Show MainMenu (with F5)
	if (IsKeyJustUp(VK_F5)) {
		Player mainPlayer = PLAYER::PLAYER_ID();
		if (firstTime){
			firstOpen(files_path_);
		}
		PLAYER::CLEAR_PLAYER_WANTED_LEVEL(mainPlayer);
		if (!menuActive)
			main_menu();
		else
			bQuit = true;
	}

	if (menuActive) {
		if (IsKeyJustUp(VK_NUMPAD5))							bEnter = true;
		if (IsKeyJustUp(VK_NUMPAD0) || IsKeyJustUp(VK_BACK))	bBack = true;
		if (IsKeyJustUp(VK_NUMPAD8))							bUp = true;
		if (IsKeyJustUp(VK_NUMPAD2))							bDown = true;
		if (IsKeyJustUp(VK_NUMPAD6))							bRight = true;
		if (IsKeyJustUp(VK_NUMPAD4))							bLeft = true;
	}


	//fly keys
	if (fly)
	{
		if (IsKeyJustUp(0x45)) //button E to go up
			ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(PLAYER::PLAYER_PED_ID(), 1, 0, 0, 2, true, true, true, true);
		if (IsKeyJustUp(0x51)) //button Q to go down
			ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(PLAYER::PLAYER_PED_ID(), 1, 0, 0, -2, true, true, true, true);
	}

	if (IsKeyJustUp(VK_SPACE)) //button SPACE to stop 
	{
		if (fly) {
			stopCoords = !stopCoords;
			char message[] = "UNLOCK";
			if (stopCoords)
				strcpy(message, "LOCK");
			fixCoords = playerCoords;
			ENTITY::SET_ENTITY_VELOCITY(PLAYER::PLAYER_PED_ID(), 0, 0, 0);
			set_status_text(message);
		}
		else if (stopCoords) {		//unlock if not flying
			stopCoords = !stopCoords;
			char message[] = "UNLOCK";
			set_status_text(message);
		}
	}
}

void ScenarioCreator::registerParams()
{
	string_params_.registerParam(this->scenario_file_param_name_);
	int_params_.registerParam(this->def_weather_param_name_);
	int_params_.registerParam(this->max_spawn_peds_param_name_);
}

void ScenarioCreator::cameraCoords()
{
	char text[100];

	if (camLock) {
		camCoords = CAM::GET_GAMEPLAY_CAM_COORD();
		camRot = CAM::GET_GAMEPLAY_CAM_ROT(2);
	}
	else {
		camCoords = CAM::GET_CAM_COORD(activeCam);
		camRot = CAM::GET_CAM_ROT(activeCam, 2);
	}

	Vector3 v = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE);
	sprintf_s(text, "player_coords: (%.3f, %.3f, %.3f)", v.x, v.y, v.z);
	//draw_text(text, 0.978, 0.205, 0.3);

	playerCoords = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE);

	v = camCoords;
	sprintf_s(text, "cam_coords: (%.3f, %.3f, %.3f)", v.x, v.y, v.z);
	//draw_text(text, 0.978 - 1 * 0.03, 0.205, 0.3);

	v = camRot;
	sprintf_s(text, "cam_rot: (%.3f, %.3f, %.3f)", v.x, v.y, v.z);
	//draw_text(text, 0.978 - 2 * 0.03, 0.205, 0.3);

}

void ScenarioCreator::main_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 8;
	menuActive = true;

	std::string caption = "EF SIMULATOR";

	static LPCSTR lineCaption[lineCount] = {
		"PEDS",
		"CAMERA",
		"PLACE",
		"TIME",
		"WEATHER",
		"INVISIBLE",
		"FLY",
		"FILE"
	};

	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;

		if (visible)
			lineCaption[5] = "INVISIBLE		~g~ON";
		else
			lineCaption[5] = "INVISIBLE		~r~OFF";

		if (fly)
			lineCaption[6] = "FLY		~g~ON";
		else
			lineCaption[6] = "FLY		~r~OFF";

		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
			if (i != activeLineIndexMain)
				draw_menu_line(lineCaption[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lineCaption[activeLineIndexMain], lineWidth, mHeight, mTopFlat + activeLineIndexMain * mTop, mLeft, mHeight, true, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();
		// process buttons
		if (bEnter)
		{
			resetMenuCommands();

			switch (activeLineIndexMain)
			{
			case 0:
				//process peds menu;
				peds_menu();
				break;
			case 1:
				//process camera menu;
				camera_menu();
				break;
			case 2:
				//process place menu;
				place_menu();
				break;
			case 3:
				//process time menu;
				time_menu();
				break;
			case 4:
				//process wweather menu;
				weather_menu();
				break;
			case 5:
				//change visibility;
				visible = !visible;
				if (visible)
					ENTITY::SET_ENTITY_VISIBLE(PLAYER::PLAYER_PED_ID(), FALSE, true);
				else
					ENTITY::SET_ENTITY_VISIBLE(PLAYER::PLAYER_PED_ID(), TRUE, true);
				break;
			case 7:
				//process file menu;
				file_menu();
				break;
			case 6:
				//change visibility;
				fly = !fly;
				Entity me = PLAYER::PLAYER_PED_ID();
				if (fly) {
					ENTITY::SET_ENTITY_HAS_GRAVITY(me, false);
					ENTITY::SET_ENTITY_COLLISION(me, FALSE, FALSE);
				}
				else {
					ENTITY::SET_ENTITY_HAS_GRAVITY(me, true);
					ENTITY::SET_ENTITY_COLLISION(me, TRUE, TRUE);
				}
				break;
			}
			waitTime = 200;
		}

		if (bBack || bQuit)
		{
			menuActive = false;
			resetMenuCommands();
			break;
		}
		else if (bUp)
		{
			if (activeLineIndexMain == 0)
				activeLineIndexMain = lineCount;
			activeLineIndexMain--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexMain++;
			if (activeLineIndexMain == lineCount)
				activeLineIndexMain = 0;
			waitTime = 150;
		}
		resetMenuCommands();
	}
}

void ScenarioCreator::peds_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 4;
	menuActive = true;

	std::string caption = "PEDS";

	char lines[lineCount][30] = {
		"NUMBER",
		"BEHAVIOUR",
		"GROUP",
		"SPAWN PEDS"
	};

	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;

		sprintf_s(lines[0], "NUMBER			 ~y~[ %d ]", nPeds);
		sprintf_s(lines[1], "BEHAVIOUR		~y~[ %s ]", tasks[currentBehaviour]);
		if (group)
			sprintf_s(lines[2], "GROUP			~y~[ ON ]");
		else
			sprintf_s(lines[2], "GROUP			~y~[ OFF ]");

		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
			if (i != activeLineIndexPeds)
				draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexPeds], lineWidth, mHeight, mTopFlat + activeLineIndexPeds * mTop, mLeft, mHeight, true, false);

			if (!subMenuActive)
				update();
			else
				return;
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();
		// process buttons
		if (bEnter)
		{
			switch (activeLineIndexPeds)
			{
			case 0:
				if (nPeds == this->max_nr_peds_per_spawn)
					nPeds = 0;
				else
					nPeds++;
				break;
			case 1:
				resetMenuCommands();
				tasks_sub_menu();
				break;
			case 2:
				group = !group;
				break;
			case 3:
				if (currentBehaviour == 8)
					spawn_peds(goFrom, nPeds);
				else
					spawn_peds(playerCoords, nPeds);
				break;
			}
			waitTime = 200;
		}
		if (bBack)
		{
			resetMenuCommands();
			break;
		}
		else if (bQuit)
		{
			resetMenuCommands();
			bQuit = true;
			break;
		}
		else if (bUp)
		{
			if (activeLineIndexPeds == 0)
				activeLineIndexPeds = lineCount;
			activeLineIndexPeds--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexPeds++;
			if (activeLineIndexPeds == lineCount)
				activeLineIndexPeds = 0;
			waitTime = 150;
		}

		if (activeLineIndexPeds == 0)
		{
			if (bLeft) {
				if (nPeds == 0)
					nPeds = this->max_nr_peds_per_spawn;
				else
					nPeds--;
			}
			else if (bRight) {
				if (nPeds == this->max_nr_peds_per_spawn)
					nPeds = 0;
				else
					nPeds++;
			}
		}
		if (activeLineIndexPeds == 1)
		{
			if (bLeft) {
				if (currentBehaviour == 0)
					currentBehaviour = numberTaks - 1;
				else
					currentBehaviour--;
			}
			else if (bRight) {
				if (currentBehaviour == numberTaks - 1)
					currentBehaviour = 0;
				else
					currentBehaviour++;
			}
		}
		resetMenuCommands();
	}
}

void ScenarioCreator::camera_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 10;
	menuActive = true;

	std::string caption = "CAMERA COORDS";

	char lines[lineCount][50] = { "", "", "", "", "", "", "", "", "", "" };
	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;

		if (camLock)
			sprintf_s(lines[0], "LOCK		~g~ON");
		else
			sprintf_s(lines[0], "LOCK		~r~OFF");

		sprintf_s(lines[1], "X			~y~[ %0.1f ]", camCoords.x);
		sprintf_s(lines[2], "Y			~y~[ %0.1f ]", camCoords.y);
		sprintf_s(lines[3], "Z			~y~[ %0.1f ]", camCoords.z);
		if (camMoving)
			sprintf_s(lines[4], "MOVING		~g~ON");
		else
			sprintf_s(lines[4], "MOVING		~r~OFF");

		sprintf_s(lines[5], "A	~y~[ x=%0.1f y=%0.1f ]", A.x, A.y);
		sprintf_s(lines[6], "B	~y~[ x=%0.1f y=%0.1f ]", B.x, B.y);
		sprintf_s(lines[7], "C	~y~[ x=%0.1f y=%0.1f ]", C.x, C.y);

		sprintf_s(lines[8], "Teleport 1	~y~[ x=%0.1f y=%0.1f ]", TP1.x, TP1.y);
		sprintf_s(lines[9], "Teleport 2	~y~[ x=%0.1f y=%0.1f ]", TP2.x, TP2.y);

		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
			if (i != activeLineIndexCamera)
				draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexCamera], lineWidth, mHeight, mTopFlat + activeLineIndexCamera * mTop, mLeft, mHeight, true, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();
		// process buttons
		if (bBack)
		{
			resetMenuCommands();
			break;
		}
		else if (bQuit)
		{
			resetMenuCommands();
			bQuit = true;
			break;
		}
		else if (bUp)
		{
			if (activeLineIndexCamera == 0)
				activeLineIndexCamera = lineCount;
			activeLineIndexCamera--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexCamera++;
			if (activeLineIndexCamera == lineCount)
				activeLineIndexCamera = 0;
			waitTime = 150;
		}
		else if (bEnter)
		{
			if (activeLineIndexCamera == 0)
			{
				camLock = !camLock;
				camLockChange();
			}
			else if (activeLineIndexCamera == 4)
			{
				camMoving = !camMoving;
			}
			else if (activeLineIndexCamera == 5)
			{
				A = playerCoords;
			}
			else if (activeLineIndexCamera == 6)
			{
				B = playerCoords;
			}
			else if (activeLineIndexCamera == 7)
			{
				C = playerCoords;
			}
			else if (activeLineIndexCamera == 8)
			{
				TP1 = playerCoords;
				TP1_rot = CAM::GET_GAMEPLAY_CAM_ROT(2);
			}
			else if (activeLineIndexCamera == 9)
			{
				TP2 = playerCoords;
				TP2_rot = CAM::GET_GAMEPLAY_CAM_ROT(2);
			}
			waitTime = 150;
		}
		else if (bLeft)
		{
			switch (activeLineIndexCamera)
			{
			case 1:
				camCoords.x -= 1;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			case 2:
				camCoords.y -= 1;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			case 3:
				camCoords.z -= 1;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			}
		}
		else if (bRight)
		{
			switch (activeLineIndexCamera)
			{
			case 1:
				camCoords.x++;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			case 2:
				camCoords.y++;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			case 3:
				camCoords.z++;
				CAM::SET_CAM_COORD(activeCam, camCoords.x, camCoords.y, camCoords.z);
				break;
			}

		}

		resetMenuCommands();
	}
}

void ScenarioCreator::place_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 16;
	menuActive = true;

	std::string caption = "PLACE";

	static struct {
		LPCSTR  text;
		float x;
		float y;
		float z;
	} lines[lineCount] = {
		{ "MICHAEL'S HOUSE", -852.4f, 160.0, 65.6f },
		{ "FRANKLIN'S HOUSE", 7.9f, 548.1f, 175.5f },
		{ "TREVOR'S TRAILER", 1985.7f, 3812.2f, 32.2f },
		{ "AIRPORT ENTRANCE", -1034.6f, -2733.6f, 13.8f },
		{ "AIRPORT FIELD", -1336.0, -3044.0f, 13.9f },
		{ "ELYSIAN ISLAND", 338.2f, -2715.9f, 38.5f },
		{ "JETSAM", 760.4f, -2943.2f, 5.8f },
		{ "STRIPCLUB", 127.4f, -1307.7f, 29.2f },
		{ "ELBURRO HEIGHTS", 1384.0f, -2057.1f, 52.0f },
		{ "FERRIS WHEEL", -1670.7f, -1125.0f, 13.0 },
		{ "CHUMASH", -3192.6f, 1100.0f, 20.2f },
		{ "WINDFARM", 2354.0f, 1830.3f, 101.1f },
		{ "MILITARY BASE", -2047.4f, 3132.1f, 32.8f },
		{ "MCKENZIE AIRFIELD", 2121.7f, 4796.3f, 41.1f },
		{ "DESERT AIRFIELD", 1747.0f, 3273.7f, 41.1f },
		{ "CHILLIAD", 425.4f, 5614.3f, 766.5f }
	};

	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;
		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
			if (i != activeLineIndexPlace)
				draw_menu_line(lines[i].text, lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexPlace].text, lineWidth, mHeight, mTopFlat + activeLineIndexPlace * mTop, mLeft, mHeight, true, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();
		// process buttons
		if (bEnter)
		{
			Entity e = PLAYER::PLAYER_PED_ID();
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e, lines[activeLineIndexPlace].x, lines[activeLineIndexPlace].y, lines[activeLineIndexPlace].z, 0, 0, 1);
		}
		else if (bBack)
		{
			resetMenuCommands();
			break;
		}
		else if (bQuit)
		{
			resetMenuCommands();
			bQuit = true;
			break;
		}
		else if (bUp)
		{
			if (activeLineIndexPlace == 0)
				activeLineIndexPlace = lineCount;
			activeLineIndexPlace--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexPlace++;
			if (activeLineIndexPlace == lineCount)
				activeLineIndexPlace = 0;
			waitTime = 150;
		}
		resetMenuCommands();
	}
}

void ScenarioCreator::time_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 2;
	menuActive = true;

	std::string caption = "TIME SKIP";

	static LPCSTR lines[lineCount] = {
		"HOUR FORWARD",
		"HOUR BACKWARD"
	};

	char timeText[32];
	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;

		sprintf_s(timeText, "~y~time %d:%d", TIME::GET_CLOCK_HOURS(), TIME::GET_CLOCK_MINUTES());

		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
			if (i != activeLineIndexTime)
				draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexTime], lineWidth, mHeight, mTopFlat + activeLineIndexTime * mTop, mLeft, mHeight, true, false);

			// show time
			draw_menu_line(timeText, lineWidth, mHeight, mTopFlat + lineCount * mTop, mLeft, mHeight, false, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();
		// process buttons
		if (bEnter)
		{
			int h = TIME::GET_CLOCK_HOURS();
			if (activeLineIndexTime == 0)
				h = (h == 23) ? 0 : h + 1;
			else
				h = (h == 0) ? 23 : h - 1;
			int m = TIME::GET_CLOCK_MINUTES();
			TIME::SET_CLOCK_TIME(h, m, 0);
			sprintf_s(timeText, "~y~time %d:%d", h, m);
			waitTime = 200;
		}
		else if (bBack)
		{
			resetMenuCommands();
			break;
		}
		else if (bQuit)
		{
			resetMenuCommands();
			bQuit = true;
			break;
		}
		else if (bUp)
		{
			if (activeLineIndexTime == 0)
				activeLineIndexTime = lineCount;
			activeLineIndexTime--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexTime++;
			if (activeLineIndexTime == lineCount)
				activeLineIndexTime = 0;
			waitTime = 150;
		}
		resetMenuCommands();
	}
}

void ScenarioCreator::weather_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 14;
	menuActive = true;

	std::string caption = "WEATHER  OPTIONS";

	static LPCSTR lines[lineCount] = {
		"WIND",
		"EXTRASUNNY",
		"CLEAR",
		"CLOUDS",
		"SMOG",
		"FOGGY",
		"OVERCAST",
		"RAIN",
		"THUNDER",
		"CLEARING",
		"NEUTRAL",
		"SNOW",
		"BLIZZARD",
		"SNOWLIGHT",
	};

	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;

		if (wind)
			lines[0] = "WIND		~g~ON";
		else
			lines[0] = "WIND		~r~OFF";

		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
			if (i != activeLineIndexWeather)
				draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexWeather], lineWidth, mHeight, mTopFlat + activeLineIndexWeather * mTop, mLeft, mHeight, true, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();
		// process buttons
		if (bEnter)
		{
			switch (activeLineIndexWeather)
			{
				// wind
			case 0:
				wind = !wind;
				if (wind)
				{
					GAMEPLAY::SET_WIND(1.0);
					GAMEPLAY::SET_WIND_SPEED(11.99f);
					GAMEPLAY::SET_WIND_DIRECTION(ENTITY::GET_ENTITY_HEADING(PLAYER::PLAYER_PED_ID()));
				}
				else
				{
					GAMEPLAY::SET_WIND(0.0);
					GAMEPLAY::SET_WIND_SPEED(0.0);
				}
				break;
				// set weather
			default:
				GAMEPLAY::CLEAR_OVERRIDE_WEATHER();
				GAMEPLAY::SET_WEATHER_TYPE_NOW_PERSIST((char *)lines[activeLineIndexWeather]);
				GAMEPLAY::CLEAR_WEATHER_TYPE_PERSIST();
				// NOTE: this line cannot be reached if activeLineIndexWeather is 0. Therefore the subtraction is valid
				weather=activeLineIndexWeather-1;
			}
			waitTime = 200;
		}
		else if (bBack)
		{
			resetMenuCommands();
			break;
		}
		else if (bQuit)
		{
			resetMenuCommands();
			bQuit = true;
			break;
		}
		else if (bUp)
		{
			if (activeLineIndexWeather == 0)
				activeLineIndexWeather = lineCount;
			activeLineIndexWeather--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexWeather++;
			if (activeLineIndexWeather == lineCount)
				activeLineIndexWeather = 0;
			waitTime = 150;
		}
		resetMenuCommands();
	}
}

void ScenarioCreator::resetMenuCommands()
{
	bEnter = false;
	bBack = false;
	bUp = false;
	bLeft = false;
	bRight = false;
	bDown = false;
	bQuit = false;
}

void ScenarioCreator::tasks_sub_menu()
{
	const float lineWidth = 320.0;
	int lineCount = 0;
	activeLineIndexSubmenu = 0;
	subMenuActive = true;

	switch (currentBehaviour)
	{
	case 0:
		lineCount = 2;
		break;
	case 1:
	case 2:
	case 3:
		lineCount = 1;
		break;
	case 4:
		lineCount = 3;
		break;
	case 8:
		lineCount = 5;
		break;
	default:
		subMenuActive = false;
		return;
		break;
	}


	char lines[5][40] = { "", "", "", "", "" };
	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;

		switch (currentBehaviour)
		{
		case 0:
			sprintf_s(lines[0], "%s			 ~y~[ %d ]", paramsLines[0].text, paramsLines[0].param);
			sprintf_s(lines[1], "%s		~y~[ %s ]", paramsLines[1].text, scenarioTypes[paramsLines[1].param]);
			break;
		case 1:
		case 2:
		case 3:
			sprintf_s(lines[0], "%s			 ~y~[ %d ]", paramsLines[0].text, paramsLines[0].param);
			break;
		case 4:
			sprintf_s(lines[0], "%s			 ~y~[ %d ]", paramsLines[2].text, paramsLines[2].param);
			sprintf_s(lines[1], "%s			 ~y~[ %d ]", paramsLines[3].text, paramsLines[3].param);
			sprintf_s(lines[2], "%s			 ~y~[ %d ]", paramsLines[4].text, paramsLines[4].param);
			break;
		case 8:
			sprintf_s(lines[0], "From	~y~[ x=%0.1f y=%0.1f ]", goFrom.x, goFrom.y);
			sprintf_s(lines[1], "To		~y~[ x=%0.1f y=%0.1f ]", goTo.x, goTo.y);
			sprintf_s(lines[2], "Speed			 ~y~[ %0.1f ]", speed);
			sprintf_s(lines[3], "%s			 ~y~[ %d ]", paramsLines[4].text, paramsLines[4].param);
			sprintf_s(lines[4], "%s			 ~y~[ %d ]", paramsLines[5].text, paramsLines[5].param);
			break;
		}

		do
		{
			// draw menu
			for (int i = 0; i < lineCount; i++)
			if (i != activeLineIndexSubmenu)
				draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft + 260, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexSubmenu], lineWidth, mHeight, mTopFlat + activeLineIndexSubmenu * mTop, mLeft + 260, mHeight, true, false);

			update();
			peds_menu();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();
		peds_menu();
		// process buttons
		if (bEnter)
		{
			if (currentBehaviour == 8)
			{
				if (activeLineIndexSubmenu == 0)
					goFrom = playerCoords;
				if (activeLineIndexSubmenu == 1)
					goTo = playerCoords;
			}
			waitTime = 200;
		}
		else if (bBack)
		{
			resetMenuCommands();
			subMenuActive = false;
			break;
		}
		else if (bQuit)
		{
			resetMenuCommands();
			subMenuActive = false;
			bQuit = true;
			break;
		}
		else if (bUp)
		{
			if (activeLineIndexSubmenu == 0)
				activeLineIndexSubmenu = lineCount;
			activeLineIndexSubmenu--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexSubmenu++;
			if (activeLineIndexSubmenu == lineCount)
				activeLineIndexSubmenu = 0;
			waitTime = 150;
		}
		else if (bLeft)
		{
			switch (currentBehaviour)
			{
			case 0:
				switch (activeLineIndexSubmenu)
				{
				case 0:
					if (paramsLines[0].param == 0)
						paramsLines[0].param = maxNumber;
					else
						paramsLines[0].param--;
					break;
				case 1:
					if (paramsLines[1].param == 0)
						paramsLines[1].param = nScenario - 1;
					else
						paramsLines[1].param--;
					break;
				}
				break;
			case 1:
			case 2:
			case 3:
				if (paramsLines[0].param == 0)
					paramsLines[0].param = maxNumber;
				else
					paramsLines[0].param--;
				break;
			case 4:
				if (paramsLines[activeLineIndexSubmenu + 2].param == 0)
					paramsLines[activeLineIndexSubmenu + 2].param = maxNumber;
				else
					paramsLines[activeLineIndexSubmenu + 2].param--;
				break;
			case 8:
				if (activeLineIndexSubmenu == 2)
				if (speed < 1.01)
					speed = 2;
				else
					speed -= 0.1f;
				if (activeLineIndexSubmenu == 3)
				if (paramsLines[4].param == 0)
					paramsLines[4].param = maxNumber;
				else
					paramsLines[4].param--;
				if (activeLineIndexSubmenu == 4)
				if (paramsLines[5].param == 0)
					paramsLines[5].param = maxNumber;
				else
					paramsLines[5].param--;
				break;
			}
		}
		else if (bRight)
		{
			switch (currentBehaviour)
			{
			case 0:
				switch (activeLineIndexSubmenu)
				{
				case 0:
					if (paramsLines[0].param == maxNumber)
						paramsLines[0].param = 0;
					else
						paramsLines[0].param++;
					break;
				case 1:
					if (paramsLines[1].param == nScenario - 1)
						paramsLines[1].param = 0;
					else
						paramsLines[1].param++;
					break;
				}
				break;
			case 1:
			case 2:
			case 3:
				if (paramsLines[0].param == maxNumber)
					paramsLines[0].param = 0;
				else
					paramsLines[0].param++;
				break;
			case 4:
				if (paramsLines[activeLineIndexSubmenu + 2].param == maxNumber)
					paramsLines[activeLineIndexSubmenu + 2].param = 0;
				else
					paramsLines[activeLineIndexSubmenu + 2].param++;
				break;
			case 8:
				if (activeLineIndexSubmenu == 2)
				if (speed > 1.99)
					speed = 1;
				else
					speed += 0.1f;
				if (activeLineIndexSubmenu == 3)
				if (paramsLines[4].param == maxNumber)
					paramsLines[4].param = 0;
				else
					paramsLines[4].param++;
				if (activeLineIndexSubmenu == 4)
				if (paramsLines[5].param == maxNumber)
					paramsLines[5].param = 0;
				else
					paramsLines[5].param++;
				break;
			}
		}
		resetMenuCommands();
	}
}

void ScenarioCreator::cancelLastLog() {
	int nchar = (int)strlen(logString);
	int n = 0;
	for (int i = nchar; i >= 0; i--) {
		if (logString[i] == '\n') {
			n++;
		}
		if (n == 2) {
			break;
		}
		logString[i] = '\0';
	}
}

void ScenarioCreator::spawn_peds(Vector3 pos, int num_ped) {

	if (nr_peds_left_ <= 0) {
		set_status_text("Cannot spawn more peds as limit given by Game is reached!");
		return;
	}

	const auto actual_nr_peds = paramsLines[5].param == -1 ? (num_ped > nr_peds_left_ ? nr_peds_left_ : num_ped) : (num_ped > static_cast<int>(nr_peds_left_ /2) ? static_cast<int>(nr_peds_left_ / 2) : num_ped);
	if (paramsLines[5].param == -1) {
		nr_peds_left_ -= actual_nr_peds;
	}
	else {
		nr_peds_left_ -= 2 * actual_nr_peds;
	}
	

	std::vector<Ped> peds;
	peds.reserve(actual_nr_peds);
	//Ped ped[100];
	//Vector3 current;
	//int i = 0;

	float rnX, rnY;
	int rn;

	writeLogLine(pos.x, pos.y, pos.z);


	// spawning 'num_ped' pedestrians
	for (int i = 0; i<actual_nr_peds; i++) {
		/*ped[i] = PED::CREATE_RANDOM_PED(pos.x, pos.y, pos.z);*/
		peds.push_back(PED::CREATE_RANDOM_PED(pos.x, pos.y, pos.z));
		WAIT(50);
	}

	overall_peds.insert(overall_peds.end(),peds.begin(),peds.end());


	// killing all pedestrians in order to prevent a bug
	/*for (int i = 0; i<num_ped; i++) {
		ENTITY::SET_ENTITY_HEALTH(ped[i], 0);
		WAIT(50);
	}*/

	for (auto& ped : peds) {
		ENTITY::SET_ENTITY_HEALTH(ped, 0);
		WAIT(50);
	}

	WAIT(500);

	int groupId = 0;
	if (group)
		groupId = PED::CREATE_GROUP(1);


	for (auto& p : peds) {

		WAIT(50);

		AI::CLEAR_PED_TASKS_IMMEDIATELY(p);
		PED::RESURRECT_PED(p);
		PED::REVIVE_INJURED_PED(p);

		// in order to prevent them from falling in hell
		ENTITY::SET_ENTITY_COLLISION(p, TRUE, TRUE);
		PED::SET_PED_CAN_RAGDOLL(p, TRUE);

		const Vector3 current = ENTITY::GET_ENTITY_COORDS(p, TRUE);

		const auto targetPed = actual_nr_peds > 1 ? *std::next(peds.begin()) : peds.front();
		/*Ped targetPed = ped[0];
		if (num_ped > 1)
			targetPed = ped[1];*/

		if (group) {
			PED::SET_PED_AS_GROUP_MEMBER(p, groupId);
			PED::SET_PED_NEVER_LEAVES_GROUP(p, true);
		}
		//srand((unsigned int)time(NULL));

		PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(p, TRUE);
		PED::SET_PED_COMBAT_ATTRIBUTES(p, 1, FALSE);
		Vector3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_ID(), TRUE);
		switch (currentBehaviour)
		{
		case 0:
			rn = rand() % 12 + 2;
			if (paramsLines[1].param == 0)
				AI::TASK_USE_NEAREST_SCENARIO_TO_COORD(p, current.x, current.y, current.z, 100.0, paramsLines[0].param);
			else if (paramsLines[1].param == 1)
				AI::TASK_START_SCENARIO_IN_PLACE(p, scenarioTypes[rn], 0, true);
			else
				AI::TASK_START_SCENARIO_IN_PLACE(p, scenarioTypes[paramsLines[1].param], 0, true);
			break;
		case 1:
			AI::TASK_STAND_STILL(p, paramsLines[0].param);
			break;
		case 2:
			AI::TASK_USE_MOBILE_PHONE_TIMED(p, paramsLines[0].param);
			//AI::TASK_START_SCENARIO_AT_POSITION(ped[i], "PROP_HUMAN_SEAT_BENCH", current.x, current.y, current.z, 0, 100000, TRUE, TRUE);
			break;
		case 3:
			AI::TASK_COWER(p, paramsLines[0].param);
			break;
		case 4:
			AI::TASK_WANDER_IN_AREA(p, current.x, current.y, current.z, (float)paramsLines[2].param, (float)paramsLines[3].param, (float)paramsLines[4].param);
			//AI::TASK_WANDER_IN_AREA(ped[i], current.x, current.y, current.z, 20.0f, 1.0f, 1.0f);
			break;
		case 5:
			if (p != peds.front())
				AI::TASK_CHAT_TO_PED(p, peds.front(), 16, 0.0, 0.0, 0.0, 0.0, 0.0);
			else
				AI::TASK_CHAT_TO_PED(p, targetPed, 16, 0.0, 0.0, 0.0, 0.0, 0.0);
			break;
		case 6:
			if (p != peds.front())
				AI::TASK_COMBAT_PED(p, peds.front(), 0, 16);
			break;
		case 7:
			AI::TASK_STAY_IN_COVER(p);
			break;
		case 8: {
			if (paramsLines[5].param == -1) {
				rnX = (float)(((rand() % 81) - 40) / 10.0);
				rnY = (float)(((rand() % 81) - 40) / 10.0);
			}
			else {
				rnX = (float)((rand() % (paramsLines[5].param * 2)) - paramsLines[5].param);
				rnY = (float)((rand() % (paramsLines[5].param * 2)) - paramsLines[5].param);
			}

			float speed_rnd = (float)(10 + rand() % 4) / 10;
			addwPed(p, coordsToVector(goFrom.x + rnX, goFrom.y + rnY, goFrom.z), coordsToVector(goTo.x + rnX, goTo.y + rnY, goTo.z), paramsLines[4].param, speed_rnd);
			Object seq;
			// waiting time proportional to distance
			float atob = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(goFrom.x, goFrom.y, goFrom.z, goTo.x, goTo.y, goTo.z, 1);
			int max_time = (int)((atob / 2.5) * 1000);
			max_time = 15000;

			AI::OPEN_SEQUENCE_TASK(&seq);
			AI::TASK_USE_MOBILE_PHONE_TIMED(0, rand() % max_time);
			AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
			AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
			AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
			AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
			AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
			AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
			AI::CLOSE_SEQUENCE_TASK(seq);
			AI::TASK_PERFORM_SEQUENCE(p, seq);
			AI::CLEAR_SEQUENCE_TASK(&seq);

			if (paramsLines[5].param != -1) {
				rnX = (float)((rand() % (paramsLines[5].param * 2)) - paramsLines[5].param);
				rnY = (float)((rand() % (paramsLines[5].param * 2)) - paramsLines[5].param);
				speed_rnd = (float)(10 + rand() % 4) / 10;

				int ped_specular = PED::CREATE_RANDOM_PED(goTo.x, goTo.y, goTo.z);
				overall_peds.push_back(ped_specular);
				WAIT(100);

				ENTITY::SET_ENTITY_HEALTH(ped_specular, 0);
				WAIT(100);
				AI::CLEAR_PED_TASKS_IMMEDIATELY(ped_specular);
				PED::RESURRECT_PED(ped_specular);
				PED::REVIVE_INJURED_PED(ped_specular);

				// in order to prevent them from falling in hell
				ENTITY::SET_ENTITY_COLLISION(ped_specular, TRUE, TRUE);
				PED::SET_PED_CAN_RAGDOLL(ped_specular, TRUE);

				PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped_specular, TRUE);
				PED::SET_PED_COMBAT_ATTRIBUTES(ped_specular, 1, FALSE);
				addwPed(ped_specular, coordsToVector(goTo.x + rnX, goTo.y + rnY, goTo.z), coordsToVector(goFrom.x + rnX, goFrom.y + rnY, goFrom.z), paramsLines[4].param, speed_rnd);

				Object seq2;
				AI::OPEN_SEQUENCE_TASK(&seq2);
				AI::TASK_USE_MOBILE_PHONE_TIMED(0, rand() % max_time);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goTo.x + rnX, goTo.y + rnY, goTo.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::TASK_GO_TO_COORD_ANY_MEANS(0, goFrom.x + rnX, goFrom.y + rnY, goFrom.z, speed_rnd, 0, 0, 786603, 0xbf800000);
				AI::CLOSE_SEQUENCE_TASK(seq2);
				AI::TASK_PERFORM_SEQUENCE(ped_specular, seq2);
				AI::CLEAR_SEQUENCE_TASK(&seq2);
			}
		}
				break;
		default:
			break;
		}

	}
}

void ScenarioCreator::walking_peds()
{
	for (int i = 0; i < nwPeds; i++)
	{
		if (PED::IS_PED_STOPPED(wPeds[i].ped) && !AI::GET_IS_TASK_ACTIVE(wPeds[i].ped, 426))
		{
			int currentTime = (TIME::GET_CLOCK_HOURS()) * 60 + TIME::GET_CLOCK_MINUTES();
			if (wPeds[i].timeFix == -1)
				wPeds[i].timeFix = currentTime;
			if (wPeds[i].timeFix + wPeds[i].stopTime < currentTime)
			{
				wPeds[i].goingTo = !wPeds[i].goingTo;
				wPeds[i].timeFix = -1;
				if (wPeds[i].goingTo)
					AI::TASK_GO_TO_COORD_ANY_MEANS(wPeds[i].ped, wPeds[i].to.x, wPeds[i].to.y, wPeds[i].to.z, wPeds[i].speed, 0, 0, 786603, 0xbf800000);
				else
					AI::TASK_GO_TO_COORD_ANY_MEANS(wPeds[i].ped, wPeds[i].from.x, wPeds[i].from.y, wPeds[i].from.z, wPeds[i].speed, 0, 0, 786603, 0xbf800000);
			}
		}
	}
}




void ScenarioCreator::file_menu()
{
	const float lineWidth = 250.0;
	const int lineCount = 4;
	menuActive = true;
	std::string caption = "FILE MANAGER";

	char lines[lineCount][50] = {
		"LOAD",
		"OVERWRITE",
		"SAVE NEW",
		"CLEAR LOG DATA"
	};
	char loadedFile[40];
	DWORD waitTime = 150;
	while (true)
	{
		// timed menu draw, used for pause after active line switch
		DWORD maxTickCount = GetTickCount() + waitTime;

		sprintf_s(loadedFile, "file:~y~ %s", fileName);

		readDirr(files_path_);

		if (nFiles == 0){
			strcpy(fileName, "None");
			sprintf_s(lines[0], "LOAD	~y~[ no files ]");
		}
		else
			sprintf_s(lines[0], "LOAD	~y~[ log_%d.txt ]", currentFile);
		
		do
		{
			// draw menu
			draw_menu_line(caption, lineWidth, mHeight, mHeight * 2, mLeft, mTitle, false, true);
			for (int i = 0; i < lineCount; i++)
			if (i != activeLineIndexFile)
				draw_menu_line(lines[i], lineWidth, mHeight, mTopFlat + i * mTop, mLeft, mHeight, false, false);
			draw_menu_line(lines[activeLineIndexFile], lineWidth, mHeight, mTopFlat + activeLineIndexFile * mTop, mLeft, mHeight, true, false);

			// show time
			draw_menu_line(loadedFile, lineWidth, mHeight, mTopFlat + lineCount * mTop, mLeft, mHeight, false, false);

			update();
			WAIT(0);
		} while (GetTickCount() < maxTickCount);
		waitTime = 0;

		update();
		// process buttons
		
		if (bEnter)
		{
			switch (activeLineIndexFile)
			{
			case 0:
				if (nFiles > 0) {
					set_status_text("Not implemented!");
					//loadFile();
				}
				break;
			case 1:
				if (strcmp(fileName, "None") != 0)
					// save file and reset nr_peds_left_ for next scenario
					nr_peds_left_ = saveFile(files_path_,default_weather_,overall_peds);
				break;
			case 2:
				sprintf_s(fileName, "log_%d.txt", nFiles + 1);
				// save file and reset nr_peds_left_ for next scenario
				nr_peds_left_ = saveFile(files_path_, default_weather_,overall_peds);
				break;
			case 3:
				strcpy(logString, "");
				set_status_text("DATA CLEARED");
				break;
			default:
				break;
			}
			waitTime = 200;
		}
		if (bBack)
		{
			resetMenuCommands();
			break;
		}
		else if (bQuit)
		{
			resetMenuCommands();
			bQuit = true;
			break;
		}
		else if (bUp)
		{
			if (activeLineIndexFile == 0)
				activeLineIndexFile = lineCount;
			activeLineIndexFile--;
			waitTime = 150;
		}
		else if (bDown)
		{
			activeLineIndexFile++;
			if (activeLineIndexFile == lineCount)
				activeLineIndexFile = 0;
			waitTime = 150;
		}
		else if (bLeft)
		{
			if (activeLineIndexFile == 0) {
				if (currentFile == 1)
					currentFile = nFiles;
				else
					currentFile--;
			}
			waitTime = 150;
		}
		else if (bRight)
		{
			if (activeLineIndexFile == 0) {
				if (currentFile == nFiles)
					currentFile = 1;
				else
					currentFile++;
			}
			waitTime = 150;
		}
		resetMenuCommands();
	}

}

void ScenarioCreator::draw_text(char *text, float x, float y, float scale) {
	UI::SET_TEXT_FONT(0);
	UI::SET_TEXT_SCALE(scale, scale);
	UI::SET_TEXT_COLOUR(255, 255, 255, 245);
	UI::SET_TEXT_WRAP(0.0, 1.0);
	UI::SET_TEXT_CENTRE(1);
	UI::SET_TEXT_DROPSHADOW(2, 2, 0, 0, 0);
	UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
	UI::_SET_TEXT_ENTRY((char*)"STRING");
	UI::_ADD_TEXT_COMPONENT_STRING(text);
	UI::_DRAW_TEXT(y, x);
}
