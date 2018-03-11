// SFML_Asteroids.cpp : Defines the entry point for the application.
//

#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <Windows.h>

#define M_PI 3.14159265358979323846

#define arrayCount(arrayToCount) sizeof(arrayToCount) / sizeof(*arrayToCount)
#define loopArray(arrayToLoop, iterator) unsigned int iterator = 0; iterator < arrayCount(arrayToLoop); iterator++
#define loopCount(count, iterator) unsigned int iterator = 0; iterator < count; iterator++

#if(_DEBUG)
	#include <iostream>
	#define debug true
	#define defineMain() main()
	#define log(var) std::cout << (var)
	#define lognl() std::cout << std::endl
#else
	#define debug false
	#define defineMain() WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
	#define log(var)
	#define lognl()
#endif

#define _removeFromArray(type, arrayPtr, index, count) \
{\
	for (unsigned int iteratorNumber = index; iteratorNumber < count - 1; iteratorNumber++)\
	{\
		type *arrayIter = arrayPtr + iteratorNumber;\
		*arrayIter = *(arrayIter + 1);\
	}\
\
	if (count > 0)\
	{\
		arrayPtr[count - 1] = {};\
	}\
}

#define removeFromArray(type, arrayPtr, index) _removeFromArray(type, arrayPtr, index, arrayCount(arrayPtr))

#define shapeToImage(type, shape, name) \
sf::RenderTexture renderTexture##name; \
{ \
	type shapeCopy(shape); \
	shapeCopy.setFillColor(sf::Color::White); \
	shapeCopy.setRotation(0); \
	shapeCopy.setOrigin(0, 0); \
	shapeCopy.setPosition(0, 0); \
	renderTexture##name.clear(); \
	renderTexture##name.create(shapeCopy.getLocalBounds().width, shapeCopy.getLocalBounds().height); \
	renderTexture##name.draw(shapeCopy); \
} \
sf::Image name = renderTexture##name.getTexture().copyToImage(); \
renderTexture##name.clear();

inline sf::IntRect FToIRect(const sf::FloatRect& f) 
{
	return sf::IntRect((int)f.left, (int)f.top, (int)f.width, (int)f.height);
}

bool PixelPerfectCollision(
	const sf::FloatRect& aBounds, const sf::FloatRect& bBounds,
	const sf::Transform& aInvTrans, const sf::Transform& bInvTrans,
	const sf::Image& imgA, const sf::Image& imgB) 
{
	sf::IntRect boundsA(FToIRect(aBounds));
	sf::IntRect boundsB(FToIRect(bBounds));
	sf::IntRect intersection;

	if (boundsA.intersects(boundsB, intersection)) 
	{
		//There are a few hacks here, sometimes the TransformToLocal returns negative points
		//Or Points outside the image.  We need to check for these as they print to the error console
		//which is slow, and then return black which registers as a hit.

		sf::Vector2i O1SubRectSize(boundsA.width, boundsA.height);
		sf::Vector2i O2SubRectSize(boundsB.width, boundsB.height);

		sf::Vector2f o1v;
		sf::Vector2f o2v;

		//Loop through our pixels
		for (int i = intersection.left; i < intersection.left + intersection.width; i++)
		{
			for (int j = intersection.top; j < intersection.top + intersection.height; j++)
			{

				o1v = aInvTrans.transformPoint(sf::Vector2f(i, j)); //Creating Objects each loop :(
				o2v = aInvTrans.transformPoint(sf::Vector2f(i, j));

				//Hack to make sure pixels fall withint the Sprite's Image
				if (o1v.x > 0 && o1v.y > 0 && o2v.x > 0 && o2v.y > 0 &&
					o1v.x < O1SubRectSize.x && o1v.y < O1SubRectSize.y &&
					o2v.x < O2SubRectSize.x && o2v.y < O2SubRectSize.y)
				{

					//If both sprites have opaque pixels at the same point we've got a hit
					if ((imgA.getPixel(static_cast<int> (o1v.x), static_cast<int> (o1v.y)).a > 0) &&
						(imgB.getPixel(static_cast<int> (o2v.x), static_cast<int> (o2v.y)).a > 0))
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

//
// Input
//

int totalPoints = 0;
int deltaMs = 0;
const int expectedMs = 16;

enum keybind_action
{
	keybind_up,
	keybind_down,
	keybind_left,
	keybind_right,
	keybind_space,
	keybind_enter,

	keybind_count,
};

struct keybind
{
	sf::Keyboard::Key key;
	const char *name;
	bool wasPressed;
	bool isPressed;
};

struct mouse
{
	sf::Vector2f pos;
	bool inWindow;
	bool wasClicked;
	bool isClicked;
};

struct input
{
	mouse mouseState;
	keybind *keyBinds;
};

void defaultKeybind(keybind *keybinds, keybind_action action, sf::Keyboard::Key defaultKey, const char *name)
{
	keybind *keybind = keybinds + action;
	keybind->key = defaultKey;
	keybind->name = name;
}

bool justPressed(keybind *keybinds, keybind_action action)
{
	keybind *keybind = keybinds + action;
	return keybind->isPressed && !keybind->wasPressed;
}

bool isPressed(keybind *keybinds, keybind_action action)
{
	keybind *keybind = keybinds + action;
	return keybind->isPressed || keybind->wasPressed;
}

bool justReleased(keybind *keybinds, keybind_action action)
{
	keybind *keybind = keybinds + action;
	return !keybind->isPressed && keybind->wasPressed;
}

bool justClicked(mouse mouseState)
{
	return mouseState.isClicked && !mouseState.wasClicked;
}

bool isClicked(mouse mouseState)
{
	return mouseState.isClicked || mouseState.wasClicked;
}

bool justReleasedClick(mouse mouseState)
{
	return !mouseState.isClicked && mouseState.wasClicked;
}

bool inWindow(mouse mouseState)
{
	return mouseState.inWindow;
}

//
// Utils
//
sf::Vector2f getForwardVector(float rotation)
{
	float forwardX = (float)sin((M_PI / 180) * rotation);
	float forwardY = (float)-cos((M_PI / 180) * rotation);
	return sf::Vector2f(forwardX, forwardY);
}
sf::Vector2f getForwardVector(sf::Transformable &entity)
{
	float rotation = entity.getRotation();
	return getForwardVector(rotation);
}
sf::Vector2f getForwardVector(sf::Transformable *entity)
{
	return getForwardVector(*entity);
}

sf::Vector2f getRightVector(float rotation)
{
	float rightX = (float)cos((M_PI / 180) * rotation);
	float rightY = (float)sin((M_PI / 180) * rotation);
	return sf::Vector2f(rightX, rightY);
}
sf::Vector2f getRightVector(sf::Transformable &entity)
{
	float rotation = entity.getRotation();
	return getRightVector(rotation);
}
sf::Vector2f getRightVector(sf::Transformable *entity)
{
	return getRightVector(*entity);
}

bool isInBounds(sf::Transformable *entity, sf::Vector2f &bounds)
{
	sf::Vector2f entityPosition = entity->getPosition();
	return !((entityPosition.x < 0) || (entityPosition.x > bounds.x)
		|| (entityPosition.y < 0) || (entityPosition.y > bounds.y));
}

void keepInBounds(sf::Transformable *entity, sf::Vector2f &bounds)
{
	sf::Vector2f entityPosition = entity->getPosition();

	if (entityPosition.x < 0)
	{
		entity->setPosition(bounds.x, entityPosition.y);
	}
	else if (entityPosition.x > bounds.x)
	{
		entity->setPosition(0, entityPosition.y);
	}
	else if (entityPosition.y < 0)
	{
		entity->setPosition(entityPosition.x, bounds.y);
	}
	else if (entityPosition.y > bounds.y)
	{
		entity->setPosition(entityPosition.x, 0);
	}
}

float processDelta(float delta)
{
	return delta *= ((float)deltaMs / (float)expectedMs);
}

sf::Vector2f processDeltaVector(sf::Vector2f delta)
{
	delta.x = processDelta(delta.x);
	delta.y = processDelta(delta.y);

	return delta;
}

//
// Events
//

struct parsed_events
{
	input *input;
};

parsed_events parseEvents(sf::RenderWindow &window, parsed_events parsedEvents)
{
	input *input = parsedEvents.input;

	input->mouseState.wasClicked = input->mouseState.isClicked;

	for (loopCount(keybind_count, i))
	{
		keybind *keyBinding = input->keyBinds + i;
		keyBinding->wasPressed = keyBinding->isPressed;
	}

	sf::Event event;
	while (window.pollEvent(event))
	{
		switch (event.type)
		{
			case sf::Event::MouseEntered:
			{
				input->mouseState.inWindow = true;
			} break;

			case sf::Event::MouseLeft:
			{
				input->mouseState.inWindow = false;
			} break;

			case sf::Event::MouseMoved:
			{
				sf::Vector2i mousePos(event.mouseMove.x, event.mouseMove.y);
				input->mouseState.pos = window.mapPixelToCoords(mousePos);
			} break;

			case sf::Event::MouseButtonPressed:
			{
				input->mouseState.isClicked = true;
			} break;

			case sf::Event::MouseButtonReleased:
			{
				input->mouseState.isClicked = false;
			} break;

			case sf::Event::KeyPressed:
			{
				for (loopCount(keybind_count, i))
				{
					keybind *keyBinding = input->keyBinds + i;
					if (keyBinding->key == event.key.code)
					{
						keyBinding->isPressed = true;
						break;
					}
				}
			} break;

			case sf::Event::KeyReleased:
			{
				for (loopCount(keybind_count, i))
				{
					keybind *keyBinding = input->keyBinds + i;
					if (keyBinding->key == event.key.code)
					{
						keyBinding->isPressed = false;
						break;
					}
				}
			} break;

			case sf::Event::Closed:
			{
				window.close();
			} break;
		}
	}

	return parsedEvents;
}

//
// Debug
//

bool debugKeys = !true;
bool debugMouse = !true;
bool debugBounds = !true;
bool debugVelocity = !true;
bool debugVectors = !true;

void drawDebugKeybinds(input &input, sf::RenderWindow &window)
{
	if (!debug || !debugKeys)
	{
		return;
	}

	keybind *keyBinds = input.keyBinds;

	if (isPressed(keyBinds, keybind_up))
	{
		sf::CircleShape circle(6);
		circle.setFillColor(sf::Color::White);
		circle.setOrigin(6, 6);
		circle.setPosition(24, 12);
		window.draw(circle);
	}
	if (isPressed(keyBinds, keybind_down))
	{
		sf::CircleShape circle(6);
		circle.setFillColor(sf::Color::White);
		circle.setOrigin(6, 6);
		circle.setPosition(24, 24);
		window.draw(circle);
	}
	if (isPressed(keyBinds, keybind_left))
	{
		sf::CircleShape circle(6);
		circle.setFillColor(sf::Color::White);
		circle.setOrigin(6, 6);
		circle.setPosition(12, 24);
		window.draw(circle);
	}
	if (isPressed(keyBinds, keybind_right))
	{
		sf::CircleShape circle(6);
		circle.setFillColor(sf::Color::White);
		circle.setOrigin(6, 6);
		circle.setPosition(36, 24);
		window.draw(circle);
	}
	if (isPressed(keyBinds, keybind_space))
	{
		sf::CircleShape circle(6);
		circle.setFillColor(sf::Color::White);
		circle.setOrigin(6, 6);
		circle.setPosition(24, 36);
		window.draw(circle);
	}
}

void drawDebugMouse(input &input, sf::RenderWindow &window)
{
	if (!debug || !debugMouse)
	{
		return;
	}

	if (inWindow(input.mouseState))
	{
		sf::CircleShape mouseCircle(2);
		mouseCircle.setOrigin(2, 2);
		mouseCircle.setFillColor(sf::Color::Magenta);
		mouseCircle.setPosition(input.mouseState.pos);
		window.draw(mouseCircle);
	}
}

void drawDebugBoundingBox(sf::RenderWindow &window, sf::FloatRect globalBounds)
{
	if (!debug || !debugBounds)
	{
		return;
	}

	sf::RectangleShape bounds(sf::Vector2f(globalBounds.width, globalBounds.height));
	bounds.setPosition(globalBounds.left, globalBounds.top);
	bounds.setFillColor(sf::Color::Transparent);
	bounds.setOutlineColor(sf::Color::Yellow);
	bounds.setOutlineThickness(2);

	window.draw(bounds);
}

void drawDebugBoundingBox(sf::RenderWindow &window, sf::Shape &entity)
{
	if (!debug || !debugBounds)
	{
		return;
	}

	sf::FloatRect globalBounds = entity.getGlobalBounds();

	drawDebugBoundingBox(window, globalBounds);
}

void drawDebugBoundingBox(sf::RenderWindow &window, sf::Text &entity)
{
	if (!debug || !debugBounds)
	{
		return;
	}

	sf::FloatRect globalBounds = entity.getGlobalBounds();

	drawDebugBoundingBox(window, globalBounds);
}

void drawDebugVelocityVector(sf::RenderWindow &window, sf::Vector2f position, sf::Vector2f velocity, float scale = 5.0f)
{
	if (!debug || !debugVelocity)
	{
		return;
	}

	velocity.x *= scale;
	velocity.y *= scale;

	sf::ConvexShape vector(4);
	vector.setPoint(0, sf::Vector2f(0, 0));
	vector.setPoint(1, sf::Vector2f(0, 0));
	vector.setPoint(2, velocity);
	vector.setPoint(3, velocity);
	vector.setPosition(position);
	vector.setOutlineColor(sf::Color::Green);
	vector.setOutlineThickness(1.0f);
	window.draw(vector);
}

void drawDebugForwardVector(sf::RenderWindow &window, sf::Transformable &entity, float scale = 20.0f)
{
	if (!debug || !debugVectors)
	{
		return;
	}

	sf::Vector2f forwardVector = getForwardVector(entity);
	forwardVector.x *= scale;
	forwardVector.y *= scale;
	sf::Vector2f position = entity.getPosition();

	sf::ConvexShape vector(4);
	vector.setPoint(0, sf::Vector2f(0, 0));
	vector.setPoint(1, sf::Vector2f(0, 0));
	vector.setPoint(2, forwardVector);
	vector.setPoint(3, forwardVector);
	vector.setPosition(position);
	vector.setOutlineColor(sf::Color::Red);
	vector.setOutlineThickness(1.0f);
	window.draw(vector);
}

void drawDebugRightVector(sf::RenderWindow &window, sf::Transformable &entity, float scale = 20.0f)
{
	if (!debug || !debugVectors)
	{
		return;
	}

	sf::Vector2f rightVector = getRightVector(entity);
	rightVector.x *= scale;
	rightVector.y *= scale;
	sf::Vector2f position = entity.getPosition();

	sf::ConvexShape vector(4);
	vector.setPoint(0, sf::Vector2f(0, 0));
	vector.setPoint(1, sf::Vector2f(0, 0));
	vector.setPoint(2, rightVector);
	vector.setPoint(3, rightVector);
	vector.setPosition(position);
	vector.setOutlineColor(sf::Color::Blue);
	vector.setOutlineThickness(1.0f);
	window.draw(vector);
}

sf::Text drawDebugButton(sf::RenderWindow &debugWindow, sf::Font &font, bool debugBool, const char *debugTextString, unsigned int index, int padding = 30)
{
	sf::Text debugText(debugTextString, font);
	debugText.setPosition(0, (float)(index * padding));
	debugText.setFillColor(debugBool ? sf::Color::Green : sf::Color::Red);
	debugWindow.draw(debugText);

	return debugText;
}

void debugButtons(sf::RenderWindow &debugWindow, sf::Font &font)
{
	debugWindow.clear();
	sf::Text debugKeysText = drawDebugButton(debugWindow, font, debugKeys, "Debug Keys", 0);
	sf::Text debugMouseText = drawDebugButton(debugWindow, font, debugMouse, "Debug Mouse", 1);
	sf::Text debugBoundsText = drawDebugButton(debugWindow, font, debugBounds, "Debug Bounds", 2);
	sf::Text debugVelocityText = drawDebugButton(debugWindow, font, debugVelocity, "Debug Velocity", 3);
	sf::Text debugVectorsText = drawDebugButton(debugWindow, font, debugVectors, "Debug Vectors", 4);
	debugWindow.display();
	sf::Event debugEvent;
	while (debugWindow.pollEvent(debugEvent))
	{
		if (debugEvent.type == sf::Event::MouseButtonPressed && debugEvent.mouseButton.button == sf::Mouse::Button::Left)
		{
			sf::Vector2f mousePos((float)debugEvent.mouseButton.x, (float)debugEvent.mouseButton.y);
#define debugTextToggle(debugText, debugBool) if(debugText.getGlobalBounds().contains(mousePos)){debugBool = !debugBool;}
			debugTextToggle(debugKeysText, debugKeys);
			debugTextToggle(debugMouseText, debugMouse);
			debugTextToggle(debugBoundsText, debugBounds);
			debugTextToggle(debugVelocityText, debugVelocity);
			debugTextToggle(debugVectorsText, debugVectors);
		}
	}
}

//
// Player
//

const float bulletSpeed = 14.5;

struct player_bullet_data
{

	sf::CircleShape bullet;
	sf::Image bulletImage;
	sf::Vector2f delta;
};

const float playerUpSpeed = 0.125f;
const float playerDownSpeed = 0.125f;
const float playerRotationSpeed = 4.25;

struct player_data
{
	sf::ConvexShape *player;
	sf::Image playerImage;
	sf::Vector2f delta;

	sf::RectangleShape *flame;
	bool flameVisible;

	player_bullet_data playerBullets[32];
	unsigned int playerBulletCount;
	
};

void anchorByOffset(sf::Transformable *parent, sf::Transformable *child, float offsetX, float offsetY)
{
	float rotation				= parent->getRotation();
	sf::Vector2f forwardVector	= getForwardVector(parent);
	sf::Vector2f rightVector	= getRightVector(parent);
	sf::Vector2f position		= parent->getPosition();

	position.x += (forwardVector.x * offsetY) + (rightVector.x * offsetX);
	position.y += (forwardVector.y * offsetY) + (rightVector.y * offsetX);

	child->setPosition(position);
	child->setRotation(rotation);
}

const float asteroidSpeed = 2.5;

struct asteroid_data
{
	sf::ConvexShape asteroid;
	sf::Image asteroidImage;
	sf::Vector2f delta;
};

int initializeAsteroids(asteroid_data *asteroids, unsigned int spawnCount, sf::Vector2f spawnPos)
{
	float size = 1.0f;

	int minSpawnCount = 5;
	int maxSpawnCount = 7;
	int spawnRotation = 0;
	int minRadius = 25;
	int maxRadius = 35;
	int spawnRadiusMin = 240;
	int spawnRadiusMax = 360;

	for (loopCount(spawnCount, i))
	{

		asteroid_data* asteroidData = asteroids + i;
		unsigned int pointCount = 16;
		int minRotation = (int)(360.0f / (float)pointCount * 0.75f);
		int maxRotation = (int)(360.0f / (float)pointCount);
		sf::ConvexShape asteroid(pointCount);

		int rotation = 0;
		for (loopCount(pointCount, j))
		{
			rotation += rand() % (maxRotation - minRotation) + minRotation;
			int radius = rand() % (maxRadius - minRadius) + minRadius;
			radius = (int)((float)radius * size);
			sf::Vector2f point = getForwardVector((float)rotation);
			point.x *= radius;
			point.y *= radius;
			asteroid.setPoint(j, sf::Vector2f(point));
		}

		spawnRotation += rand() % (40) + 40;
		int radius = rand() % (spawnRadiusMax - spawnRadiusMin) + spawnRadiusMin;
		sf::Vector2f position = getForwardVector((float)spawnRotation);
		float deltaRotation = (float)(rand() % (360 * 5)) / 5;
		sf::Vector2f delta = getForwardVector(deltaRotation);
		delta.x *= asteroidSpeed;
		delta.y *= asteroidSpeed;

		asteroid.setFillColor(sf::Color::Transparent);
		asteroid.setOutlineThickness(1);
		asteroid.setOutlineColor(sf::Color::White);
		position.x *= radius;
		position.y *= radius;
		position.x += spawnPos.x;
		position.y += spawnPos.y;
		asteroid.setPosition(position);
		asteroid.setRotation(deltaRotation);
		shapeToImage(sf::ConvexShape, asteroid, asteroidImage);
		asteroidData->asteroidImage = asteroidImage;
		asteroidData->asteroid = asteroid;
		asteroidData->delta = delta;
	}

	return spawnCount;
}

void processPlayer(input &input, player_data &playerData, sf::Vector2f &bounds, asteroid_data* asteroids, unsigned int *asteroidCount)
{
	keybind *keybinds			= input.keyBinds;
	sf::ConvexShape *player		= playerData.player;
	sf::RectangleShape *flame	= playerData.flame;

	float deltaVelocity = 0;
	float rotation = player->getRotation();

	playerData.flameVisible = false;

	if (justPressed(keybinds, keybind_enter))
	{
		player->setRotation(rotation + 180);
	}

	if (isPressed(keybinds, keybind_left))
	{
		player->rotate(-playerRotationSpeed);
	}
	if (isPressed(keybinds, keybind_right))
	{
		player->rotate(playerRotationSpeed);
	}

	if (isPressed(keybinds, keybind_up))
	{
		deltaVelocity = processDelta(playerUpSpeed);
	}

	sf::Vector2f forwardVector = getForwardVector(player);
	playerData.delta.x += forwardVector.x * deltaVelocity;
	playerData.delta.y += forwardVector.y * deltaVelocity;

	if (isPressed(keybinds, keybind_down))
	{
		float dx = playerData.delta.x;
		float dy = playerData.delta.y;
		deltaVelocity = processDelta(playerDownSpeed);

		playerData.delta.x = dx > 0 ? (dx - deltaVelocity < 0 ? 0 : dx - deltaVelocity) : (dx + deltaVelocity > 0 ? 0 : dx + deltaVelocity);
		playerData.delta.y = dy > 0 ? (dy - deltaVelocity < 0 ? 0 : dy - deltaVelocity) : (dy + deltaVelocity > 0 ? 0 : dy + deltaVelocity);
	}

	keepInBounds(player, bounds);
	player->move(playerData.delta);

	if (isPressed(keybinds, keybind_up))
	{
		playerData.flameVisible = true;
		anchorByOffset(player, flame, 0, -13);
	}

	if (justPressed(keybinds, keybind_space) && playerData.playerBulletCount < arrayCount(playerData.playerBullets))
	{
		player_bullet_data *playerBullet = playerData.playerBullets + playerData.playerBulletCount++;

		const unsigned int bulletSize = 2;
		playerBullet->bullet = sf::CircleShape((float)bulletSize);
		playerBullet->bullet.setOrigin((float)bulletSize, (float)bulletSize);
		playerBullet->bullet.setPosition(player->getPosition());
		playerBullet->bullet.setRotation(rotation);
		playerBullet->bullet.setFillColor(sf::Color::Red);
		shapeToImage(sf::CircleShape, playerBullet->bullet, bulletImage);
		playerBullet->bulletImage = bulletImage;

		playerBullet->delta.x = playerData.delta.x + (forwardVector.x * processDelta(bulletSpeed));
		playerBullet->delta.y = playerData.delta.y + (forwardVector.y * processDelta(bulletSpeed));
	}

	for (loopCount(playerData.playerBulletCount, i))
	{
		player_bullet_data *playerBullet = playerData.playerBullets + i;

		if (!isInBounds(&playerBullet->bullet, bounds))
		{
			_removeFromArray(player_bullet_data, playerData.playerBullets, i, playerData.playerBulletCount);
			playerData.playerBulletCount -= 1;
			continue;
		}
		playerBullet->bullet.move(processDeltaVector(playerBullet->delta));

		for (loopCount(*asteroidCount, j))
		{
			asteroid_data* asteroid = asteroids + j;
			sf::CircleShape* bulletShape = &playerBullet->bullet;
			sf::FloatRect bulletBounds = bulletShape->getGlobalBounds();
			sf::ConvexShape* asteroidShape = &asteroid->asteroid;
			sf::FloatRect asteroidBounds = asteroidShape->getGlobalBounds();
			if (bulletBounds.intersects(asteroidBounds))
			{
				asteroid_data* asteroid = asteroids + j;
				totalPoints += 1;

				_removeFromArray(player_bullet_data, playerData.playerBullets, i, playerData.playerBulletCount);
				_removeFromArray(asteroid_data, asteroids, j, *asteroidCount);

				playerData.playerBulletCount -= 1;
				*asteroidCount -= 1;
			}
		}
	}
}

void processAsteroids(asteroid_data* asteroids, unsigned int *asteroidCount, sf::Vector2f &bounds, unsigned int* spawnCount, sf::Vector2f spawnPos)
{
	for (loopCount(*asteroidCount, i))
	{
		asteroid_data* asteroid = asteroids + i;
		asteroid->asteroid.move(asteroid->delta);
		keepInBounds(&asteroid->asteroid, bounds);
	}

	if (*asteroidCount <= 0)
	{
		*spawnCount = (int)((float)(*spawnCount) * 1.25f);
		*asteroidCount = initializeAsteroids(asteroids, *spawnCount, spawnPos);
	}
}

//
// Main
//

int defineMain()
{
#if(debug)
	sf::RenderWindow debugWindow(sf::VideoMode(800, 600), "Debug");
#endif

	sf::RenderWindow window(sf::VideoMode(800, 600), "Asteroids");
	window.setFramerateLimit(60);

	sf::Vector2f bounds = window.getView().getSize();

	keybind keybinds[keybind_count] = {};
	defaultKeybind(keybinds, keybind_up, sf::Keyboard::W, "Move Forward");
	defaultKeybind(keybinds, keybind_down, sf::Keyboard::S, "Move Backwards");
	defaultKeybind(keybinds, keybind_left, sf::Keyboard::A, "Turn Left");
	defaultKeybind(keybinds, keybind_right, sf::Keyboard::D, "Turn Right");
	defaultKeybind(keybinds, keybind_space, sf::Keyboard::Space, "Fire");
	defaultKeybind(keybinds, keybind_enter, sf::Keyboard::E, "Enter");

	input input		= {};
	input.keyBinds	= keybinds;

	parsed_events parsedEvents	= {};
	parsedEvents.input			= &input;

	while (window.isOpen())
	{

		const unsigned int playerSize = 25;
		sf::ConvexShape player(4);
		player.setPoint(0, sf::Vector2f(((float)playerSize / 2), 0));
		player.setPoint(1, sf::Vector2f(0, (float)playerSize));
		player.setPoint(2, sf::Vector2f(((float)playerSize / 2), ((float)playerSize) * 0.8f));
		player.setPoint(3, sf::Vector2f((float)playerSize, (float)playerSize));
		player.setOrigin(((float)playerSize / 2), ((float)playerSize / 2));
		player.setFillColor(sf::Color::Transparent);
		player.setOutlineThickness(1);
		player.setOutlineColor(sf::Color::White);
		player.setPosition(400, 300);

		const unsigned int flameSize = 10;
		sf::RectangleShape flame(sf::Vector2f((float)flameSize, (float)flameSize));
		flame.setOrigin(((float)flameSize / 2), ((float)flameSize / 2));
		flame.setFillColor(sf::Color::Red);

		player_data playerData = {};
		playerData.player = &player;
		playerData.flame = &flame;
		shapeToImage(sf::ConvexShape, player, playerImage);
		playerData.playerImage = playerImage;

		unsigned int spawnCount = 8;
		asteroid_data asteroids[32] = {};
		unsigned int asteroidCount = initializeAsteroids(asteroids, spawnCount, player.getPosition());

		sf::Font font;
		font.loadFromFile("arial.ttf");

		bool gameOver = false;
		bool restart = false;

		while (!restart && window.isOpen())
		{
			sf::Vector2f debugPos(0.0f, 0.0f);

			restart = false;

			sf::Clock clock = sf::Clock();
#if(debug)
			//debugButtons(debugWindow, font);
#endif
			parsedEvents = parseEvents(window, parsedEvents);

			if (!gameOver)
			{
				sf::FloatRect playerBounds = player.getGlobalBounds();

				for (loopCount(asteroidCount, i))
				{
					asteroid_data* asteroid = asteroids + i;
					sf::FloatRect asteroidBounds = asteroid->asteroid.getGlobalBounds();
					if (asteroidBounds.intersects(playerBounds))
					{
						totalPoints = 0;
						gameOver = true;
					}
				}
			}

			if (gameOver)
			{
				sf::Text gameOverText("Game Over!", font);
				gameOverText.setOrigin(sf::Vector2f(gameOverText.getGlobalBounds().width / 2, gameOverText.getGlobalBounds().height / 2));
				gameOverText.setPosition(window.getView().getCenter());
				window.draw(gameOverText);
				window.display();

				if (justPressed(keybinds, keybind_enter))
				{
					restart = true;
				}

				continue;
			}

			processPlayer(input, playerData, bounds, asteroids, &asteroidCount);
			processAsteroids(asteroids, &asteroidCount, bounds, &spawnCount, player.getPosition());

			debugWindow.clear();
			window.clear();

#define renderImageDebug(image, object) \
{ \
	sf::Texture imageTexture; \
	imageTexture.loadFromImage(image); \
	sf::Sprite imageSprite(imageTexture); \
	imageSprite.setRotation(object.getRotation()); \
	imageSprite.setOrigin(object.getOrigin()); \
	imageSprite.setPosition(object.getPosition()); \
	debugWindow.draw(imageSprite); \
}

			// Game Render Start
			{
				for (loopCount(playerData.playerBulletCount, i))
				{
					player_bullet_data *playerBullet = playerData.playerBullets + i;
					window.draw(playerBullet->bullet);
					drawDebugBoundingBox(window, playerBullet->bullet);
					drawDebugVelocityVector(window, playerBullet->bullet.getPosition(), playerBullet->delta);
					drawDebugForwardVector(window, playerBullet->bullet);
					drawDebugRightVector(window, playerBullet->bullet);

					renderImageDebug(playerBullet->bulletImage, playerBullet->bullet);
				}

				if (playerData.flameVisible)
				{
					window.draw(flame);
					drawDebugBoundingBox(window, flame);
					drawDebugForwardVector(window, flame);
					drawDebugRightVector(window, flame);
				}

				for (loopCount(asteroidCount, i))
				{
					asteroid_data *asteroidData = asteroids + i;
					window.draw(asteroidData->asteroid);
					drawDebugBoundingBox(window, asteroidData->asteroid);
					drawDebugVelocityVector(window, asteroidData->asteroid.getPosition(), asteroidData->delta);
					drawDebugForwardVector(window, asteroidData->asteroid);
					drawDebugRightVector(window, asteroidData->asteroid);

					renderImageDebug(asteroidData->asteroidImage, asteroidData->asteroid);
				}

				window.draw(player);
				drawDebugBoundingBox(window, player);
				drawDebugVelocityVector(window, player.getPosition(), playerData.delta, 8.0F);
				drawDebugForwardVector(window, player);
				drawDebugRightVector(window, player);

				renderImageDebug(playerData.playerImage, player);

				char buffer[16];
				sprintf_s(buffer, "Points: %i", totalPoints);
				sf::Text pointsText = sf::Text(buffer, font);
				pointsText.setOrigin(pointsText.getGlobalBounds().width / 2, pointsText.getGlobalBounds().height / 2);
				sf::Vector2f position = window.getView().getCenter();
				position.y = window.getView().getSize().y / 8;
				pointsText.setPosition(position);
				window.draw(pointsText);

				drawDebugKeybinds(input, window);
				drawDebugMouse(input, window);
			}
			// Game Render End

			window.display();
			debugWindow.display();
			deltaMs = clock.getElapsedTime().asMilliseconds();
		}
	}

	return 0;
}