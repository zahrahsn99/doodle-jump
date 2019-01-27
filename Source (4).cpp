#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL.h>                   // SDL library
#include <cmath>                   // abs()
#include <ctime>                   // rand()
#include <string> 
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
using namespace std;


typedef struct {
	double arrX, arrY;
	int width, height;
	int type;
	int score;
} BrickObject;

vector<BrickObject> Bricks;

double brspeed = .5;
int brickDistance = 50;

SDL_Window*     window;                 // holds window properties
SDL_Renderer*   renderer;               // holds rendering surface properties

SDL_Color dark_font = { 67, 68, 69 };   // dark grey
  									    
int SCREEN_WIDTH = 480;                 // Screen resolution  
int SCREEN_HEIGHT = 640;

const int BRICK_WIDTH = 100;
const int BRICK_HEIGHT = 10;

int LEVEL = 0;
bool start = false;

bool gameover = false;
bool resetDoodle = true;

unsigned int timer = 0;
int score = 0;

#pragma region Ltexture
//Texture wrapper class
class LTexture
{
public:
	//Initializes variables
	LTexture();

	//Deallocates memory
	~LTexture();

	//Loads image at specified path
	bool loadFromFile(std::string path);

#ifdef _SDL_TTF_H
	//Creates image from font string
	bool loadFromRenderedText(std::string textureText, SDL_Color textColor);
#endif

	//Deallocates texture
	void free();

	//Set color modulation
	void setColor(Uint8 red, Uint8 green, Uint8 blue);

	//Set blending
	void setBlendMode(SDL_BlendMode blending);

	//Set alpha modulation
	void setAlpha(Uint8 alpha);

	//Renders texture at given point
	void render(int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

	//Gets image dimensions
	int getWidth();
	int getHeight();

private:
	//The actual hardware texture
	SDL_Texture* mTexture;

	//Image dimensions
	int mWidth;
	int mHeight;
};

LTexture::LTexture()
{
	//Initialize
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::~LTexture()
{
	//Deallocate
	free();
}

bool LTexture::loadFromFile(std::string path)
{
	//Get rid of preexisting texture
	free();

	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		//Color key image
		SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));

		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
		if (newTexture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	//Return success
	mTexture = newTexture;
	return mTexture != NULL;
}

void LTexture::free()
{
	//Free texture if it exists
	if (mTexture != NULL)
	{
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
	//Modulate texture rgb
	SDL_SetTextureColorMod(mTexture, red, green, blue);
}

void LTexture::setBlendMode(SDL_BlendMode blending)
{
	//Set blending function
	SDL_SetTextureBlendMode(mTexture, blending);
}

void LTexture::setAlpha(Uint8 alpha)
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod(mTexture, alpha);
}

void LTexture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, mWidth, mHeight };

	//Set clip rendering dimensions
	if (clip != NULL)
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx(renderer, mTexture, clip, &renderQuad, angle, center, flip);
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}
//Scene textures
LTexture DoodleChar;
#pragma endregion

#pragma region Doodle
class Doodle
{
public:
	//The dimensions of the Doodle
	static const int Doodle_WIDTH = 50;
	static const int Doodle_HEIGHT = 49;

	//Initializes the variables
	Doodle();

	//Moves the Doodle and checks collision
	void move();

	//Shows the Doodle on the screen
	void render();

	void reset();

	void toLeft();
	void toRight();
	void setspeed();

	void jump();
	void bigJump();

private:
	//The X and Y offsets of the Doodle
	double mPosX, mPosY;

	//The doodle cant jump more than two times in a row without jumping to a brick
	int jumpcount = 0;

	//The doodle direction
	int dir = 0;

	//gravity
	int g = 1;

	//Can doodle jump again?
	bool canjump = true;

	//The velocity of the Doodle
	double mVelX, mVelY;

	//Doodle's collision box
	SDL_Rect mCollider;
};

Doodle::Doodle()
{
	//Initialize the offsets
	mPosX = (SCREEN_WIDTH - Doodle_WIDTH) / 2;
	mPosY = SCREEN_HEIGHT - Doodle_HEIGHT;

	//Set collision box dimension
	mCollider.w = Doodle_WIDTH;
	mCollider.h = Doodle_HEIGHT;

	//Initialize the velocity
	mVelX = 0;
	mVelY = 0;

}

void Doodle::reset()
{
	//Initialize the offsets
	mPosX = (SCREEN_WIDTH - Doodle_WIDTH) / 2;
	mPosY = SCREEN_HEIGHT - Doodle_HEIGHT -1;

	//Initialize the velocity
	setspeed();
}

void Doodle::toRight()
{
	if(mVelX<4)
		mVelX += 1;
}

void Doodle::toLeft()
{
	if(mVelX>-4)
		mVelX += -1;
}

void Doodle::jump()
{
	
	if (mVelY > 0 && jumpcount < 2 && canjump || resetDoodle)
	{
		resetDoodle = false;
		g = 1;
		mVelY = -20;
		jumpcount++;
		if (jumpcount > 1)
		{
			canjump = false;
		}
	}
}

void Doodle::bigJump()
{
	if (mVelY > 0 && jumpcount < 2)
	{
		g = 1;
		mVelY = -35;
		jumpcount++;
	}
}

void Doodle::setspeed()
{
	mVelX = 0;
	mVelY = 0;
}

void Doodle::move()
{
	//Move the doodle left or right
	mPosX += mVelX;
	mCollider.x = mPosX;

	//Changes the direction of the doodle
	if (mVelX > 0)
	{
		dir = 0;
	}
	else {
		dir = 1;
	}

	//If the doodle collides with bottom of the screen the game will reset
	if (mPosY + Doodle_HEIGHT > SCREEN_HEIGHT)
	{
		resetDoodle = true;
		start = false;
	}


	//If the doodle collided or went too far to the left
	if (mPosX < 0)
	{
		//Move back
		mVelX = abs(mVelX);
		mCollider.x += mVelX + 1;
		
	}

	//If the doodle collided or went too far to the right
	if (mPosX + Doodle_WIDTH > SCREEN_WIDTH)
	{
		//Move back
		mVelX = -1 * abs(mVelX);
		mCollider.x += mVelX - 1;
	}

	//Move the doodle up or down (projectile motion)
	if (!resetDoodle)
	{
		mVelY += g;
		mPosY += mVelY;
		mCollider.y = mPosY;
	}
	

	int topDoodle, bottomDoodle, posBall;
	int rightBrick, leftBrick, topBrick, bottomBrick;

	topDoodle = mCollider.y;
	bottomDoodle = mCollider.y + Doodle_HEIGHT;
	posBall = mCollider.x;

	//Check if the doodle is on top of a brick or collide with alein
	for (int i = 0; i < Bricks.size(); i++)
	{
		leftBrick = Bricks.at(i).arrX;
		rightBrick = Bricks.at(i).arrX + BRICK_WIDTH;
		topBrick = Bricks.at(i).arrY;
		bottomBrick = Bricks.at(i).arrY + BRICK_HEIGHT;


		//Collision with alein
		if (Bricks.at(i).type == 6 )
		{
			SDL_Rect inter;
			SDL_Rect br = { Bricks.at(i).arrX+7,Bricks.at(i).arrY+7,Bricks.at(i).width-14,Bricks.at(i).height -14};
			SDL_bool check = SDL_IntersectRect(&br, &mCollider,&inter);
			if (check == SDL_TRUE)
			{
				resetDoodle = true;
				start = false;
				Bricks.at(i).type = 0;
			}
		}
		if (bottomDoodle >= topBrick && topDoodle < topBrick -10 && posBall <= rightBrick- Doodle_WIDTH/2 && posBall >= leftBrick - Doodle_WIDTH/2 && mVelY>0 && Bricks.at(i).type !=5 && Bricks.at(i).type != 6 && Bricks.at(i).type != 0)
		{
			score += Bricks.at(i).score ;
			Bricks.at(i).score = 0;
		
			mPosY = topBrick - Doodle_HEIGHT;
			g = 0;
			mVelY = brspeed;
			mCollider.y += mVelY;
			jumpcount = 0;
			canjump = true;
			if (Bricks.at(i).type == 2)
			{
				Bricks.at(i).type = 1;
				bigJump();
			}
		}
		else {
			g = 1;
		}
	}
	
}

void Doodle::render()
{
	//Show the doodle according to direction
	if(dir == 1)
		DoodleChar.render(mPosX, mPosY,NULL,0,NULL,SDL_FLIP_HORIZONTAL);
	else
		DoodleChar.render(mPosX, mPosY);
}

#pragma endregion

Doodle doodle;
void update()
{
	if (start)
	{
		doodle.move();
	}
}

SDL_Texture *renderText(const string &msg, const string &fontPath, SDL_Color color, int fontSize, SDL_Renderer *ren) {
	TTF_Font *font = TTF_OpenFont(fontPath.c_str(), fontSize);
	string t = TTF_GetError();
	if (font == nullptr) {
		return nullptr;
	}
	
	SDL_Surface *surface = TTF_RenderText_Blended(font, msg.c_str(), color);
	if (surface == nullptr) {
		TTF_CloseFont(font);
		return nullptr;
	}

	SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surface);

	SDL_FreeSurface(surface);
	TTF_CloseFont(font);

	return tex;
}

void renderTexture(SDL_Texture *tex , SDL_Renderer *ren, int x, int y, int w = -1, int h = -1) {
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	dest.w = w;
	dest.h = h;

	// If no width and height are specified, use the texture's actual width and height
	if (w == -1 || h == -1)
		SDL_QueryTexture(tex, NULL, NULL, &dest.w, &dest.h);

	SDL_RenderCopy(ren, tex, NULL, &dest);
}

void draw_bricks(SDL_Renderer *ren)
{
	for (int i = 0; i < Bricks.size(); i++)
	{
		if (Bricks[i].arrY < SCREEN_HEIGHT)
		{
			LTexture brick;
			if (Bricks[i].type == 1)
			{	
				brick.loadFromFile("br1.png");
			}else if (Bricks[i].type == 2)
			{
				brick.loadFromFile("br2.png");
			}
			else if (Bricks[i].type == 5)
			{
				brick.loadFromFile("br5.png");
			}
			else if (Bricks[i].type == 6)
			{
				brick.loadFromFile("alein.png");
			}
			Bricks[i].height = brick.getHeight();
			Bricks[i].width = brick.getWidth();
			brick.render(Bricks[i].arrX, Bricks[i].arrY);
		}
	}
}

BrickObject createRandomBrick(int x,int y,bool obsticle)
{
	BrickObject brick = { x ,y };
	brick.arrX = x;
	brick.arrY = y;
	if (rand() % 20 > 17)
	{
		brick.type = 2;
		brick.score = 30;
	}
	else
	{
		brick.type = 1;
		brick.score = 5;
	}
	if (rand() % 20 > 16 && obsticle)
	{
		brick.type = 5;
	}
	if (rand() % 20 > 16 && obsticle)
	{
		brick.type = 6;
	}
	return brick;
}

void update_board()
{	
	for (auto it = Bricks.begin(); it != Bricks.end(); ++it)
	{
		it->arrY += brspeed;
	}
	if (timer% int(brickDistance/brspeed) == 0)
	{
		int brx = rand() % 380;
		int bry = 20;
		Bricks.push_back(createRandomBrick(brx,bry,true));
	}
}

// Render objects on screen
void render() {
	timer++;
	if (timer % 100 == 0 && start == true)
	{
		brspeed += .1;
	}

	// Clear screen (background color)
	SDL_SetRenderDrawColor(renderer, 227, 231, 240, 255);        // dark grey
	SDL_RenderClear(renderer);

	// Top line
	SDL_SetRenderDrawColor(renderer, 212, 120, 102, 255);
	SDL_Rect topLine = { 0 ,0, SCREEN_WIDTH,40 };
	SDL_RenderFillRect(renderer, &topLine);

	SDL_Texture *p1score = renderText("Score: " + std::to_string(score), "fonts/sample.ttf", dark_font, 25, renderer);
	renderTexture(p1score, renderer, 20, 9);
	SDL_DestroyTexture(p1score);

	update_board();
	if (!start)
	{
		brspeed = .5;
		doodle.reset();
	}
	draw_bricks(renderer);
	doodle.render();

	// Swap buffers
	SDL_RenderPresent(renderer);
	
}

void cleanUp() {

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	// Shuts down SDL
	SDL_Quit();
}

void addbricks()
{
	Bricks.clear();
	for (int i = 1; i < 10; i++) {
		int brx = rand() % 380;
		int bry = i * 50;
		Bricks.push_back(createRandomBrick(brx, bry, false));
	}
}

bool quit = false;
void gameLoop() {
	SDL_Event e;
	
	while (!quit) {
		while (SDL_PollEvent(& e)) {
			const Uint8 *keyboard_state_array = SDL_GetKeyboardState(NULL);
			if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
			{

				if (keyboard_state_array[SDL_SCANCODE_RIGHT] && !keyboard_state_array[SDL_SCANCODE_LEFT])
				{
					start = true;
					doodle.toRight();
				}
				else if (!keyboard_state_array[SDL_SCANCODE_RIGHT] && keyboard_state_array[SDL_SCANCODE_LEFT])
				{
					start = true;
					doodle.toLeft();
				}

				if (keyboard_state_array[SDL_SCANCODE_SPACE])
				{
					start = true;
					doodle.jump();
				}

				if (keyboard_state_array[SDL_SCANCODE_ESCAPE])
				{
					quit = true;
				}
			}


			if (e.type == SDL_QUIT)  quit = true;
		}
		update();
		render();
	}

	cleanUp();
}

bool loadMedia()
{
	//Loading success flag
	bool success = true;

	//Load doodle char
	if (!DoodleChar.loadFromFile("djc.png"))
	{
		printf("Failed to load Doodle texture!\n");
		success = false;
	}
	return success;
}

void initialize() {

	// Initialize SDL
	SDL_Init(SDL_INIT_EVERYTHING);

	// Create window in the middle of the screen
	window = SDL_CreateWindow("Bricks",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN);
	addbricks();
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	TTF_Init();

	loadMedia();
}

int main(int argc, char *argv[]) {
	srand(time(NULL));
	initialize();
	gameLoop();
	return 0;
}