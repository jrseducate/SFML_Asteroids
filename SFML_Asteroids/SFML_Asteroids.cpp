// SFML_Asteroids.cpp : Defines the entry point for the application.
//

#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <Windows.h>

#define M_PI 3.14159265358979323846

#define arrayCount(arrayToCount) sizeof(arrayToCount) / sizeof(*arrayToCount)
#define loopArray(arrayToLoop, iterator) unsigned int iterator = 0; iterator < arrayCount(arrayToLoop); iterator++
#define loopCount(count, iterator) int iterator = 0; iterator < count; iterator++

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
	renderTexture##name.clear(); \
	renderTexture##name.create((unsigned int)shapeCopy.getGlobalBounds().width, (unsigned int)shapeCopy.getGlobalBounds().height); \
	shapeCopy.setPosition(shapeCopy.getGlobalBounds().width / 2, shapeCopy.getGlobalBounds().height / 2); \
	renderTexture##name.draw(shapeCopy); \
} \
sf::Image name = renderTexture##name.getTexture().copyToImage(); \
name.flipVertically(); \
renderTexture##name.clear();

int deltaMs = 0;
const int expectedMs = 15;

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
	return getRightVector(entity.getRotation());
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

float getDistance(sf::Vector2f A, sf::Vector2f B)
{
	float xDiff = A.x - B.x;
	float yDiff = A.y - B.y;

	float distance = sqrt(pow(xDiff, 2) + pow(yDiff, 2));

	return distance;
}

sf::Vector2f getRotatedPosition(float rotation, sf::Vector2f position, sf::Vector2f offsetPosition)
{
	sf::Vector2f forwardVector = getForwardVector(rotation);
	sf::Vector2f rightVector = getRightVector(rotation);

	position.x += (forwardVector.x * offsetPosition.y) + (rightVector.x * offsetPosition.x);
	position.y += (forwardVector.y * offsetPosition.y) + (rightVector.y * offsetPosition.x);

	return position;
}

//
// Input
//

int totalPoints = 0;

enum keybind_action
{
	keybind_up,
	keybind_down,
	keybind_left,
	keybind_right,
	keybind_space,
	keybind_enter,
	keybind_pause,
	keybind_debug,

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
bool debugCollision = !true;
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

void drawDebugCollisionCircles(sf::RenderWindow &window, sf::Transformable &entity, sf::Vector2f *points, int pointCount)
{
	if (!debug || !debugCollision)
	{
		return;
	}

	float radius = 2;

	for (loopCount(pointCount, i))
	{
		sf::Vector2f point = points[i];

		point = getRotatedPosition(entity.getRotation(), entity.getPosition(), point);

		sf::CircleShape collisionCircle(radius);
		collisionCircle.setOrigin(radius / 2, radius / 2);
		collisionCircle.setPosition(point);
		collisionCircle.setFillColor(sf::Color::Transparent);
		collisionCircle.setOutlineColor(sf::Color::Cyan);
		collisionCircle.setOutlineThickness(2);
		window.draw(collisionCircle);
	}
}

void drawDebugCollisionCircle(sf::RenderWindow &window, sf::Vector2f position, float radius)
{
	if (!debug || !debugCollision)
	{
		return;
	}

	sf::CircleShape collisionCircle(radius);
	collisionCircle.setOrigin(radius, radius);
	collisionCircle.setPosition(position);
	collisionCircle.setFillColor(sf::Color::Transparent);
	collisionCircle.setOutlineColor(sf::Color::Cyan);
	collisionCircle.setOutlineThickness(2);
	window.draw(collisionCircle);
}

void drawDebugCollisionCircle(sf::RenderWindow &window, sf::Shape &entity, float radius)
{
	drawDebugCollisionCircle(window, entity.getPosition(), radius);
}

void drawDebugBoundingBox(sf::RenderWindow &window, sf::FloatRect globalBounds, sf::Color color = sf::Color::Yellow)
{
	if (!debug || !debugBounds)
	{
		return;
	}

	sf::RectangleShape bounds(sf::Vector2f(globalBounds.width, globalBounds.height));
	bounds.setPosition(globalBounds.left, globalBounds.top);
	bounds.setFillColor(sf::Color::Transparent);
	bounds.setOutlineColor(color);
	bounds.setOutlineThickness(2);

	window.draw(bounds);
}

void drawDebugBoundingBox(sf::RenderWindow &window, sf::Shape &entity, sf::Color color = sf::Color::Yellow)
{
	drawDebugBoundingBox(window, entity.getGlobalBounds(), color);
}

void drawDebugBoundingBox(sf::RenderWindow &window, sf::Sprite &entity, sf::Color color = sf::Color::Yellow)
{
	drawDebugBoundingBox(window, entity.getGlobalBounds(), color);
}

void drawDebugBoundingBox(sf::RenderWindow &window, sf::Text &entity, sf::Color color = sf::Color::Yellow)
{
	drawDebugBoundingBox(window, entity.getGlobalBounds(), color);
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
	if (debug)
	{
		debugWindow.clear();
		sf::Text debugKeysText = drawDebugButton(debugWindow, font, debugKeys, "Debug Keys", 0);
		sf::Text debugMouseText = drawDebugButton(debugWindow, font, debugMouse, "Debug Mouse", 1);
		sf::Text debugBoundsText = drawDebugButton(debugWindow, font, debugBounds, "Debug Bounds", 2);
		sf::Text debugCollisionText = drawDebugButton(debugWindow, font, debugCollision, "Debug Collision", 3);
		sf::Text debugVelocityText = drawDebugButton(debugWindow, font, debugVelocity, "Debug Velocity", 4);
		sf::Text debugVectorsText = drawDebugButton(debugWindow, font, debugVectors, "Debug Vectors", 5);
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
				debugTextToggle(debugCollisionText, debugCollision);
				debugTextToggle(debugVelocityText, debugVelocity);
				debugTextToggle(debugVectorsText, debugVectors);
			}
		}
	}
}

//
// Player
//

const float bulletSpeed = 12.25;

struct player_bullet_data
{

	sf::CircleShape bullet;
	sf::Vector2f delta;
	float collisionRadius;
};

const float playerUpSpeed = 0.125f;
const float playerDownSpeed = 0.125f;
const float playerRotationSpeed = 4.25;

struct player_data
{
	sf::ConvexShape *player;
	sf::Vector2f delta;

	sf::Vector2f* collisionPoints;
	int collisionPointCount;

	sf::RectangleShape *flame;
	bool flameVisible;

	player_bullet_data playerBullets[32];
	unsigned int playerBulletCount;
	
};

void anchorByOffset(sf::Transformable *parent, sf::Transformable *child, float offsetX, float offsetY)
{
	float rotation				= parent->getRotation();
	sf::Vector2f position		= parent->getPosition();

	position = getRotatedPosition(rotation, position, sf::Vector2f(offsetX, offsetY));

	child->setPosition(position);
	child->setRotation(rotation);
}

const float asteroidSpeed = 2.5;

struct asteroid_data
{
	sf::ConvexShape asteroid;
	sf::Vector2f delta;
	float collisionRadius;
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
		int outerRadius = 0;
		for (loopCount(pointCount, j))
		{
			rotation += rand() % (maxRotation - minRotation) + minRotation;
			int radius = rand() % (maxRadius - minRadius) + minRadius;
			radius = (int)((float)radius * size);
			outerRadius = (radius > outerRadius) ? radius : outerRadius;
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
		asteroidData->asteroid = asteroid;
		asteroidData->delta = delta;
		asteroidData->collisionRadius =(float) outerRadius;
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
		player->rotate(-processDelta(playerRotationSpeed));
	}
	if (isPressed(keybinds, keybind_right))
	{
		player->rotate(processDelta(playerRotationSpeed));
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
	player->move(processDeltaVector(playerData.delta));

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

		playerBullet->delta.x = playerData.delta.x + (forwardVector.x * bulletSpeed);
		playerBullet->delta.y = playerData.delta.y + (forwardVector.y * bulletSpeed);
		playerBullet->collisionRadius = (float)bulletSize;
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
				float distance = getDistance(asteroid->asteroid.getPosition(), bulletShape->getPosition());
				if (distance < (asteroid->collisionRadius + playerBullet->collisionRadius))
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
}

void processAsteroids(asteroid_data* asteroids, unsigned int *asteroidCount, sf::Vector2f &bounds, unsigned int* spawnCount, sf::Vector2f spawnPos)
{
	for (loopCount(*asteroidCount, i))
	{
		asteroid_data* asteroid = asteroids + i;
		asteroid->asteroid.move(processDeltaVector(asteroid->delta));
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
	defaultKeybind(keybinds, keybind_pause, sf::Keyboard::Escape, "Pause");
	defaultKeybind(keybinds, keybind_debug, sf::Keyboard::Q, "Debug");

	input input		= {};
	input.keyBinds	= keybinds;

	parsed_events parsedEvents	= {};
	parsedEvents.input			= &input;

	bool debugToggle = false;

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

		const int playerCollisionPointCount = 3;
		sf::Vector2f playerCollisionPoints[playerCollisionPointCount];

		playerCollisionPoints[0] = sf::Vector2f(0, ((float)playerSize / 2));
		playerCollisionPoints[1] = sf::Vector2f(-((float)playerSize / 2), -((float)playerSize / 2));
		playerCollisionPoints[2] = sf::Vector2f(((float)playerSize / 2), -((float)playerSize / 2));

		const unsigned int flameSize = 10;
		sf::RectangleShape flame(sf::Vector2f((float)flameSize, (float)flameSize));
		flame.setOrigin(((float)flameSize / 2), ((float)flameSize / 2));
		flame.setFillColor(sf::Color::Red);

		player_data playerData = {};
		playerData.player = &player;
		playerData.collisionPoints = playerCollisionPoints;
		playerData.collisionPointCount = playerCollisionPointCount;
		playerData.flame = &flame;

		unsigned int spawnCount = 8;
		asteroid_data asteroids[32] = {};
		unsigned int asteroidCount = initializeAsteroids(asteroids, spawnCount, player.getPosition());

		sf::Font font;
		font.loadFromFile("arial.ttf");

		bool paused = false;
		bool gameOver = false;
		bool restart = false;

		while (!restart && window.isOpen())
		{
			if (justPressed(keybinds, keybind_debug))
			{
				debugToggle = !debugToggle;
			}

			window.clear();
#if(debug)
			debugWindow.clear();
#endif

			if (justPressed(keybinds, keybind_pause))
			{
				paused = !paused || gameOver;
			}

			if (!debugToggle)
			{
				debugButtons(debugWindow, font);
			}

			sf::Vector2f debugPos(0.0f, 0.0f);

			restart = false;

			sf::Clock clock;

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
						bool asteroidHitPlayer = false;
						sf::Vector2f asteroidPosition = asteroid->asteroid.getPosition();
						for (loopArray(playerCollisionPoints, i))
						{
							sf::Vector2f playerCollisionPoint = playerCollisionPoints[i];
							playerCollisionPoint.x += player.getPosition().x;
							playerCollisionPoint.y += player.getPosition().y;

							float distance = getDistance(asteroidPosition, playerCollisionPoint);

							if (distance < asteroid->collisionRadius)
							{
								asteroidHitPlayer = true;
								break;
							}
						}

						if (asteroidHitPlayer)
						{
							gameOver = true;
						}
					}
				}
			}

			if (gameOver)
			{
				paused = true;

				sf::Text gameOverText("Game Over!", font);
				gameOverText.setOrigin(sf::Vector2f(gameOverText.getGlobalBounds().width / 2, gameOverText.getGlobalBounds().height / 2));
				gameOverText.setPosition(window.getView().getCenter());
				window.draw(gameOverText);

				if (justPressed(keybinds, keybind_enter))
				{
					restart = true;
					totalPoints = 0;
				}
			}

			if (!paused)
			{
				processPlayer(input, playerData, bounds, asteroids, &asteroidCount);
				processAsteroids(asteroids, &asteroidCount, bounds, &spawnCount, player.getPosition());
			}

#if debug
#define renderImageDebug(image, object) \
{ \
	sf::Texture imageTexture; \
	imageTexture.loadFromImage(image); \
	sf::Sprite imageSprite(imageTexture); \
	imageSprite.setRotation(object.getRotation()); \
	imageSprite.setOrigin(object.getOrigin()); \
	imageSprite.setPosition(object.getPosition()); \
	drawDebugBoundingBox(debugWindow, imageSprite, sf::Color::Magenta); \
	debugWindow.draw(imageSprite); \
}
#else
	#define renderImageDebug(image, object)
#endif
			// Game Render Start
			{
				for (loopCount(playerData.playerBulletCount, i))
				{
					player_bullet_data *playerBullet = playerData.playerBullets + i;
					window.draw(playerBullet->bullet);
					drawDebugBoundingBox(window, playerBullet->bullet);
					drawDebugCollisionCircle(window, playerBullet->bullet, playerBullet->collisionRadius);
					drawDebugVelocityVector(window, playerBullet->bullet.getPosition(), playerBullet->delta);
					drawDebugForwardVector(window, playerBullet->bullet);
					drawDebugRightVector(window, playerBullet->bullet);
					drawDebugForwardVector(debugWindow, playerBullet->bullet);
					drawDebugRightVector(debugWindow, playerBullet->bullet);
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
					drawDebugCollisionCircle(window, asteroidData->asteroid, asteroidData->collisionRadius);
					drawDebugVelocityVector(window, asteroidData->asteroid.getPosition(), asteroidData->delta);
					drawDebugForwardVector(window, asteroidData->asteroid);
					drawDebugRightVector(window, asteroidData->asteroid);
					drawDebugForwardVector(debugWindow, asteroidData->asteroid);
					drawDebugRightVector(debugWindow, asteroidData->asteroid);
				}

				window.draw(player);
				drawDebugBoundingBox(window, player);
				drawDebugCollisionCircles(window, player, playerData.collisionPoints, playerData.collisionPointCount);
				drawDebugVelocityVector(window, player.getPosition(), playerData.delta, 8.0F);
				drawDebugForwardVector(window, player);
				drawDebugRightVector(window, player);
				drawDebugForwardVector(debugWindow, player);
				drawDebugRightVector(debugWindow, player);

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
#if(debug)
			debugWindow.display();
#endif
			deltaMs = clock.getElapsedTime().asMilliseconds();
		}
	}

	return 0;
}