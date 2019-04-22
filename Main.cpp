#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <math.h> 
#include <random>

using namespace sf;

// Some constansts, note that they are unit-less
#define GRAVITY 4
#define THRUSTER_FORCE 10
#define INITIAL_HORIZONTAL_SPEED 200000

#define MAX_HORIZONTAL_LANDING_SPEED 70000
#define MAX_VERTICAL_LANDING_SPEED 60000

// g++ -std=c++17 Main.cpp -lsfml-graphics -lsfml-window -lsfml-system

std::ostream& operator<<(std::ostream& os, const Vector2f& v)
{
    os << "[" << v.x << ", " << v.y << "]";
    return os;
}

/**
 * Represents the terrain as a series of connected lines
 *
 **/
class Terrain : public VertexArray
{
public:
    Terrain(float _width, float startY, float _maxY, float _minY) : 
        VertexArray(PrimitiveType::LinesStrip),
        width(_width),
        maxY(_maxY), 
        minY(_minY),
        generator(std::random_device{}())

    {   
        AddVertex(0, startY);
    }

    void Update(double xVelocity)
    {

        for (int i = 0; i < getVertexCount(); i++)
            (*this)[i].position.x -= 0.00000001f * xVelocity;

        GenerateTerrain();
    }

private:
    void AddVertex(float x, float y)
    {
        append(Vertex(Vector2f(x, y), Color::White));
    }

    /**
     * Ensures that we have enough terrain points to cover the current viewable area
     */
    void GenerateTerrain()
    {
        std::uniform_int_distribution<int> xDist(20, 40);
        std::uniform_int_distribution<int> yDist(-80, 80);
        std::uniform_int_distribution<int> isFlat(0, 4);

        while (true)
        {
            const Vertex& lastPoint = (*this)[getVertexCount() - 1];

            if (lastPoint.position.x >= width)
            {
                break;
            }

            float deltaY = isFlat(generator) == 0 ? 0 : yDist(generator);
            float newX = lastPoint.position.x + xDist(generator);
            float newY = lastPoint.position.y + deltaY;
            newY = std::max(minY, std::min(newY, maxY));

            std::cout << "newY = " << newY << " (with delta " << deltaY << ") before adjusting for min/max = " << minY << "/" << maxY << std::endl;

            AddVertex(newX, newY);
        }
    }

private:
    float width;
    float maxY;
    float minY;
    float startY;
    std::default_random_engine generator;
};

/**
 * The lander 
 **/
class Lander : public Sprite
{
public:
    Lander()
    {
        if (!texture.loadFromFile("lander.png"))
            throw "Error loading lander texture";

        setTexture(texture);
        setOrigin(getLocalBounds().width / 2, getLocalBounds().height / 2);
    }

    void UpdatePosition()
    {
        Vector2f boosterForce;
        if (thrustersOn)
        {
            float rotationRad = getRotation() * (2 * 3.14159f) / 360.f;
            boosterForce = {THRUSTER_FORCE * sin(rotationRad), -THRUSTER_FORCE * cos(rotationRad)};
        }

        Vector2f totalForce = gravityForce + boosterForce;

        currentSpeed += totalForce; // Assume mass = 1

        Vector2f newPos = getPosition() + 0.00000001f * currentSpeed;
        newPos.x = getPosition().x;

        /*std::cout << "DeltaT = " << deltaTime << 
            ", gravityForce = " << gravityForce << 
            ", boosterForce = " << boosterForce << 
            ", totalForce = " << totalForce << 
            ", currentSpeed = " << currentSpeed << 
            ", newPos = " << newPos << 
            std::endl;*/

        setPosition(newPos);
    }

    void setThrustersOn(bool b) 
    {
        thrustersOn = b;
    }

    const Vector2f& GetCurrentSpeed() const 
    {
        return currentSpeed;
    }

private:
    bool thrustersOn = false;

    sf::Texture texture;

    const Vector2f gravityForce { 0, GRAVITY };
    
    Vector2f currentSpeed { INITIAL_HORIZONTAL_SPEED, 0 };
};


// Check if the line from (x1, y1) to (x2, y2) intersects with the line from (x3, y3) to (x4, y4)
// From http://www.jeffreythompson.org/collision-detection/line-line.php
bool LinesIntersect(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    float uA = ((x4-x3)*(y1-y3) - (y4-y3)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));
    float uB = ((x2-x1)*(y1-y3) - (y2-y1)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));

    if (uA >= 0 && uA <= 1 && uB >= 0 && uB <= 1) 
        return true;

    return false;
}

bool Crashed(const Vector2f& from, const Vector2f& to, Lander& lander)
{
    // Must land on a float surface
    if (abs(from.y - to.y) > 2)
    {
        std::cout << "Crashed: from.y = " << from.y << ", to.y = " << to.y << std::endl;
        return true;
    }

    if (lander.GetCurrentSpeed().x > MAX_HORIZONTAL_LANDING_SPEED) 
    {
        std::cout << "Crashed, horizontal speed: " << lander.GetCurrentSpeed().x << " > " << MAX_HORIZONTAL_LANDING_SPEED << std::endl;
        return true;
    }

    if (lander.GetCurrentSpeed().y > MAX_VERTICAL_LANDING_SPEED) 
    {
        std::cout << "Crashed, vertical speed: " << lander.GetCurrentSpeed().y << " > " << MAX_VERTICAL_LANDING_SPEED << std::endl;
        return true;
    }

    std::cout << "No crashed,speed: " << lander.GetCurrentSpeed() << std::endl;

    return false;
}

/**
 * main
 */
int main()
{
    RenderWindow window(sf::VideoMode(800, 600), "Lunar lander");

    window.setPosition(sf::Vector2i(
        sf::VideoMode::getDesktopMode().width * 0.5 - window.getSize().x * 0.5, 
        sf::VideoMode::getDesktopMode().height * 0.5 - window.getSize().y * 0.5));

    Lander lander;
    lander.setPosition(window.getSize().x / 2, window.getSize().y / 3);

    Terrain terrain(
        window.getSize().x, 
        3 * window.getSize().y / 4, 
        window.getSize().y - 20,
        window.getSize().y / 5);

    bool running = true;

    sf::Font font;
    font.loadFromFile("arial.ttf");

    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(30);
    text.setStyle(sf::Text::Bold);
    

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            switch (event.type) 
            {

                case Event::Closed:
                    window.close();
                    break;
                case Event::KeyPressed:
                    
                    if (running)
                    {
                        if (event.key.code == Keyboard::Key::Left) {
                            lander.rotate(-2);
                        }
                        else if (event.key.code == Keyboard::Key::Right) {
                            lander.rotate(2);
                        }
                        else if (event.key.code == Keyboard::Key::Space) {
                            lander.setThrustersOn(true);
                        }
                    }
                    break;
                case Event::KeyReleased:
                    if (running)
                    {
                        if (event.key.code == Keyboard::Key::Space) {
                            lander.setThrustersOn(false);
                        }
                    }
                    break;
                case Event::Resized:
                    FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                    window.setView(View(visibleArea));
                    break;
            }
        }


        window.clear();
        window.draw(lander);
        window.draw(terrain);

        if (!running)
        {
            window.draw(text);
        }

        window.display();

        if (running)
        {
            lander.UpdatePosition();
            terrain.Update(lander.GetCurrentSpeed().x);

            
            Vector2f from = terrain[0].position;
            FloatRect lr = lander.getGlobalBounds();

            bool intersects;

            for (int i = 1; i < terrain.getVertexCount(); i++)
            {
                Vector2f to = terrain[i].position;

                intersects = 
                    LinesIntersect(from.x, from.y, to.x, to.y, lr.left, lr.top, lr.left + lr.width, lr.top) ||
                    LinesIntersect(from.x, from.y, to.x, to.y, lr.left + lr.width, lr.top, lr.left + lr.width, lr.top + lr.height) ||
                    LinesIntersect(from.x, from.y, to.x, to.y, lr.left + lr.width, lr.top + lr.height, lr.left, lr.top + lr.height) ||
                    LinesIntersect(from.x, from.y, to.x, to.y, lr.left, lr.top + lr.height, lr.left, lr.top);

                if (intersects)
                {
                    running = false;
                    if (Crashed(from, to, lander))
                    {
                        text.setFillColor(sf::Color::Red);
                        text.setString("Oh no you crashed!");
                    } 
                    else
                    {
                        text.setFillColor(sf::Color::Green);
                        text.setString("Good job commander, you landed the lunar lander!");
                    }


                    break;   
                }
                from = to;
            }
        }
    }

    return 0;
}
