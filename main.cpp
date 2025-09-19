#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <vector>
#include <random>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include"./include/ssr.hpp"


int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SSR Simulation");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
    {
        if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf") &&
            !font.loadFromFile("/System/Library/Fonts/Arial.ttf"))
        {
            return -1;
        }
    }

    // Get multiplicative factor from user
    float multiplicativeFactor{1.0f};

    std::vector<BoxInfo> boxes;
    boxes.reserve(BOX_COUNT);
    createDescendingEdges(boxes, font);

    std::vector<HistogramBar> histogram;
    histogram.reserve(BOX_COUNT - 1);
    createHistogram(histogram, font);

    sf::Text histTitle, yAxisLabel, xAxisLabel;

    yAxisLabel.setFont(font);
    yAxisLabel.setCharacterSize(10);
    yAxisLabel.setFillColor(sf::Color::White);
    yAxisLabel.setString("Count");
    yAxisLabel.setPosition(5, 120);
    yAxisLabel.setRotation(-90);

    xAxisLabel.setFont(font);
    xAxisLabel.setCharacterSize(10);
    xAxisLabel.setFillColor(sf::Color::White);
    xAxisLabel.setString("State");

    // Center the x-axis label under the histogram
    const float histX = 20.0f;
    const float histWidth = 900.0f;
    sf::FloatRect xLabelBounds = xAxisLabel.getLocalBounds();
    float xLabelX = histX + (histWidth - xLabelBounds.width) / 2;
    xAxisLabel.setPosition(xLabelX, 240);

    // Add labels in top right
    sf::Text stateCountLabel, stepCountLabel, ballCountLabel, factorLabel, cycleCountLabel;

    stateCountLabel.setFont(font);
    stateCountLabel.setCharacterSize(16);
    stateCountLabel.setFillColor(sf::Color::White);
    stateCountLabel.setString("States: " + std::to_string(BOX_COUNT - 1));
    stateCountLabel.setPosition(WINDOW_WIDTH - 160, 20);

    stepCountLabel.setFont(font);
    stepCountLabel.setCharacterSize(16);
    stepCountLabel.setFillColor(sf::Color(200, 255, 200));
    stepCountLabel.setString("Steps: 0");
    stepCountLabel.setPosition(WINDOW_WIDTH - 160, 42);

    ballCountLabel.setFont(font);
    ballCountLabel.setCharacterSize(16);
    ballCountLabel.setFillColor(sf::Color(255, 200, 200));
    ballCountLabel.setString("Balls: 1");
    ballCountLabel.setPosition(WINDOW_WIDTH - 160, 64);

    factorLabel.setFont(font);
    factorLabel.setCharacterSize(16);
    factorLabel.setFillColor(sf::Color(200, 200, 255));
    factorLabel.setString("Factor: " + std::to_string(multiplicativeFactor).substr(0, 4));
    factorLabel.setPosition(WINDOW_WIDTH - 160, 86);

    cycleCountLabel.setFont(font);
    cycleCountLabel.setCharacterSize(16);
    cycleCountLabel.setFillColor(sf::Color(255, 255, 200));
    cycleCountLabel.setString("Cycles: 0");
    cycleCountLabel.setPosition(WINDOW_WIDTH - 160, 108);

    int stepCount = 0;
    int cycleCount = 0;

    // Add speed control variables
    float simulationSpeed = 1.0f;
    const float minSpeed = 0.1f;
    const float maxSpeed = 10.0f;
    float waitTime = 1.5f;
    float jumpDurationBase = 0.8f;
    bool isPaused = false;

    sf::Text speedLabel;
    speedLabel.setFont(font);
    speedLabel.setCharacterSize(13);
    speedLabel.setFillColor(sf::Color(255, 215, 0));
    speedLabel.setString("Speed: 1.0x");
    speedLabel.setPosition(WINDOW_WIDTH - 400, 264);

    sf::Text pauseLabel;
    pauseLabel.setFont(font);
    pauseLabel.setCharacterSize(13);
    pauseLabel.setFillColor(sf::Color(255, 100, 100));
    pauseLabel.setString("");
    pauseLabel.setPosition(WINDOW_WIDTH - 400, 280);

    // Initialize balls vector
    std::vector<Ball> balls;
    sf::Vector2f initialPos;
    initialPos.x = boxes[0].x * SCALE + BOX_WIDTH * SCALE / 2.f;
    initialPos.y = WINDOW_HEIGHT - boxes[0].height * SCALE - BALL_RADIUS * SCALE;
    balls.push_back(Ball(initialPos, 0));

    const size_t maxTrailSize = 50;
    sf::CircleShape ballShape(BALL_RADIUS * SCALE);
    ballShape.setOrigin(BALL_RADIUS * SCALE, BALL_RADIUS * SCALE);

    sf::Clock frameTimer;
    sf::Clock cycleWaitTimer;
    std::mt19937 rng(std::random_device{}());
    const float cycleWaitTime = 2.0f; // Wait 2 seconds between cycles

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::S)
                {
                    sf::Texture texture;
                    texture.create(window.getSize().x, window.getSize().y);
                    texture.update(window);

                    auto now = std::time(nullptr);
                    std::tm *tm_ptr = std::localtime(&now);
                    std::ostringstream oss;
                    oss << "screenshot_" << std::put_time(tm_ptr, "%Y%m%d_%H%M%S") << ".png";

                    sf::Image screenshot = texture.copyToImage();
                    screenshot.saveToFile(oss.str());
                    std::cout << "Screenshot saved as " << oss.str() << "\n";
                }
                else if (event.key.code == sf::Keyboard::R)
                {
                    resetSimulation(boxes, histogram, balls, stepCount, cycleCount, boxes);
                    stepCountLabel.setString("Steps: 0");
                    ballCountLabel.setString("Balls: 1");
                    cycleCountLabel.setString("Cycles: 0");
                    cycleWaitTimer.restart();
                    std::cout << "Simulation reset\n";
                }
                else if (event.key.code == sf::Keyboard::F)
                {
                    multiplicativeFactor = getUserMultiplicativeFactor();
                    factorLabel.setString("Factor: " + std::to_string(multiplicativeFactor).substr(0, 4));
                    resetSimulation(boxes, histogram, balls, stepCount, cycleCount, boxes);
                }
                else if (event.key.code == sf::Keyboard::Equal || event.key.code == sf::Keyboard::Add)
                {
                    simulationSpeed = std::min(simulationSpeed + 0.5f, maxSpeed);
                    speedLabel.setString("Speed: " + std::to_string(simulationSpeed).substr(0, 3) + "x");
                    std::cout << "Speed increased to " << simulationSpeed << "x\n";
                }
                else if (event.key.code == sf::Keyboard::Hyphen || event.key.code == sf::Keyboard::Subtract)
                {
                    simulationSpeed = std::max(simulationSpeed - 0.5f, minSpeed);
                    speedLabel.setString("Speed: " + std::to_string(simulationSpeed).substr(0, 3) + "x");
                    std::cout << "Speed decreased to " << simulationSpeed << "x\n";
                }
                else if (event.key.code == sf::Keyboard::Space)
                {
                    isPaused = !isPaused;
                    pauseLabel.setString(isPaused ? "PAUSED" : "");
                    std::cout << (isPaused ? "Simulation paused\n" : "Simulation resumed\n");
                }
            }
        }

        if (!isPaused)
        {
            float dt = frameTimer.restart().asSeconds();
            float adjustedWaitTime = waitTime / simulationSpeed;
            float adjustedJumpDuration = jumpDurationBase / simulationSpeed;

            // Check if all balls have reached the end
            bool allBallsAtEnd = true;
            for (const auto &ball : balls)
            {
                if (!ball.hasReachedEnd)
                {
                    allBallsAtEnd = false;
                    break;
                }
            }

            // If all balls have reached the end, wait and start new cycle
            if (allBallsAtEnd && balls.size() > 0)
            {
                if (cycleWaitTimer.getElapsedTime().asSeconds() > cycleWaitTime / simulationSpeed)
                {
                    startNewCycle(balls, boxes, cycleCount);
                    cycleCountLabel.setString("Cycles: " + std::to_string(cycleCount));
                    cycleWaitTimer.restart();
                }
            }
            else
            {
                // Process all balls
                std::vector<Ball> newBalls;

                for (auto &ball : balls)
                {
                    // Update trail first
                    ball.trail.push_back(ball.position);
                    if (ball.trail.size() > maxTrailSize)
                        ball.trail.erase(ball.trail.begin());

                    if (ball.hasReachedEnd)
                    {
                        // Ball has reached the end, just keep it there
                        newBalls.push_back(ball);
                        continue;
                    }

                    if (!ball.isJumping && ball.timer.getElapsedTime().asSeconds() > adjustedWaitTime)
                    {
                        if (ball.currentBox > 0)
                        {
                            boxes[ball.currentBox].visits++;
                            boxes[ball.currentBox].countText.setString(std::to_string(boxes[ball.currentBox].visits));
                            updateHistogram(histogram, boxes);
                            stepCount++;
                        }

                        if (ball.currentBox == BOX_COUNT - 1)
                        {
                            // Ball reached the end
                            ball.hasReachedEnd = true;
                            newBalls.push_back(ball);
                        }
                        else
                        {
                            // Split the ball based on multiplicative factor
                            std::vector<Ball> splitBalls = splitBall(ball, multiplicativeFactor, boxes, rng);

                            // Setup jump for each new ball
                            for (auto &newBall : splitBalls)
                            {
                                std::uniform_int_distribution<int> dist(ball.currentBox + 1, BOX_COUNT - 1);
                                newBall.nextBox = dist(rng);
                                newBall.startPos = ball.position;
                                newBall.targetPos.x = boxes[newBall.nextBox].x * SCALE + BOX_WIDTH * SCALE / 2.f;
                                newBall.targetPos.y = WINDOW_HEIGHT - boxes[newBall.nextBox].height * SCALE - BALL_RADIUS * SCALE;
                                newBall.isJumping = true;
                                newBall.jumpProgress = 0.0f;
                                newBall.timer.restart();
                            }

                            newBalls.insert(newBalls.end(), splitBalls.begin(), splitBalls.end());
                        }
                    }
                    else if (ball.isJumping)
                    {
                        ball.jumpProgress += dt / adjustedJumpDuration;
                        if (ball.jumpProgress >= 1.0f)
                        {
                            ball.jumpProgress = 1.0f;
                            ball.position = ball.targetPos;
                            ball.currentBox = ball.nextBox;
                            ball.isJumping = false;
                            ball.timer.restart();
                        }
                        else
                        {
                            float t = smoothStep(ball.jumpProgress);
                            ball.position.x = ball.startPos.x + (ball.targetPos.x - ball.startPos.x) * t;
                            float baseY = ball.startPos.y + (ball.targetPos.y - ball.startPos.y) * t;
                            float arcHeight = 100.0f;
                            float arcOffset = -arcHeight * 4.0f * ball.jumpProgress * (1.0f - ball.jumpProgress);
                            ball.position.y = baseY + arcOffset;
                        }

                        newBalls.push_back(ball);
                    }
                    else
                    {
                        newBalls.push_back(ball);
                    }
                }

                balls = std::move(newBalls);

                // Update display labels
                ballCountLabel.setString("Balls: " + std::to_string(balls.size()));
                stepCountLabel.setString("Steps: " + std::to_string(stepCount));

                // Limit number of balls to prevent performance issues
                if (balls.size() > 1000)
                {
                    balls.erase(balls.begin() + 1000, balls.end());
                    std::cout << "Limited balls to 1000 for performance\n";
                }
            }
        }
        else
        {
            frameTimer.restart(); // Keep timer in sync when paused
        }

        // Rendering
        window.clear(sf::Color::Black);

        window.draw(histTitle);
        window.draw(yAxisLabel);
        window.draw(xAxisLabel);


        window.draw(stateCountLabel);
        window.draw(stepCountLabel);
        window.draw(ballCountLabel);
        window.draw(factorLabel);
        window.draw(cycleCountLabel);

        for (const auto &histBar : histogram)
        {
            window.draw(histBar.bar);
            window.draw(histBar.label);
            if (histBar.bar.getSize().y > 10)
                window.draw(histBar.valueText);
        }

        for (const auto &box : boxes)
        {
            window.draw(box.shape);
            window.draw(box.countText);
        }

        // Draw all balls and their trails
        for (const auto &ball : balls)
        {
            // Draw trail with fading effect
            for (size_t i = 0; i < ball.trail.size(); ++i)
            {
                float alpha = static_cast<float>(i) / ball.trail.size() * 120;
                sf::CircleShape trailDot(BALL_RADIUS * SCALE / 2.5f);
                trailDot.setOrigin(trailDot.getRadius(), trailDot.getRadius());
                trailDot.setPosition(ball.trail[i]);
                sf::Color trailColor = ball.color;
                trailColor.a = static_cast<sf::Uint8>(alpha);
                trailDot.setFillColor(trailColor);
                window.draw(trailDot);
            }

            // Draw ball
            ballShape.setPosition(ball.position);
            ballShape.setFillColor(ball.color);
            window.draw(ballShape);
        }

        window.display();
    }

    return 0;
}