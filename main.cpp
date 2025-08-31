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

const float SCALE = 30.f;
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int BOX_COUNT = 21.f;
const float BOX_WIDTH = (WINDOW_WIDTH - (BOX_COUNT + 1) * 2.f) / SCALE / BOX_COUNT;
const float BALL_RADIUS = 0.5f;

struct Ball
{
    sf::Vector2f position;
    sf::Vector2f startPos;
    sf::Vector2f targetPos;
    int currentBox = 0;
    int nextBox = 0;
    bool isJumping = false;
    float jumpProgress = 0.0f;
    sf::Clock timer;
    std::vector<sf::Vector2f> trail;
    sf::Color color = sf::Color::Green;
    bool hasReachedEnd = false;

    // Default constructor
    Ball()
    {
        trail.reserve(50);
    }

    // Parameterized constructor
    Ball(sf::Vector2f pos, int box, sf::Color col = sf::Color::Green)
        : position(pos), currentBox(box), color(col), hasReachedEnd(false)
    {
        trail.reserve(50);
    }
};

struct BoxInfo
{
    sf::RectangleShape shape;
    int visits = 0;
    sf::Text countText;
    float x;
    float height;
    int index;
};

struct HistogramBar
{
    sf::RectangleShape bar;
    sf::Text label;
    sf::Text valueText;
};

void createDescendingEdges(std::vector<BoxInfo> &boxes, sf::Font &font)
{
    float gap = 2.f / SCALE;
    for (int i = 0; i < BOX_COUNT; ++i)
    {
        float x = i * (BOX_WIDTH + gap);
        float height = (BOX_COUNT - i) * 0.6f;

        sf::RectangleShape rect(sf::Vector2f(BOX_WIDTH * SCALE, height * SCALE));
        rect.setPosition(x * SCALE, (WINDOW_HEIGHT - height * SCALE));
        rect.setFillColor(sf::Color::White);
        rect.setOutlineColor(sf::Color::Red);
        rect.setOutlineThickness(2.f);

        sf::Text label;
        // label.setFont(font);
        // label.setCharacterSize(16);
        // label.setFillColor(sf::Color::Black);
        // label.setPosition(x * SCALE + 5, (WINDOW_HEIGHT - height * SCALE) + 5);
        // label.setString("0");

        boxes.push_back(BoxInfo{rect, 0, label, x, height, i});
    }
}

float smoothStep(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

void createHistogram(std::vector<HistogramBar> &histogram, sf::Font &font)
{
    const float histX = 20.0f;
    const float histY = 40.0f;
    const float histWidth = 900.0f;
    const float histHeight = 180.0f;
    const float barWidth = histWidth / (BOX_COUNT - 1);

    for (int i = 0; i < BOX_COUNT - 1; ++i)
    {
        HistogramBar histBar;
        histBar.bar.setSize(sf::Vector2f(barWidth - 2.0f, 0.0f));
        histBar.bar.setPosition(histX + i * barWidth, histY + histHeight);
        histBar.bar.setFillColor(sf::Color(100, 150, 255, 180));
        histBar.bar.setOutlineColor(sf::Color::White);
        histBar.bar.setOutlineThickness(1.0f);

        // Display labels in ascending order (1, 2, 3, ..., 20) from left to right
        // But keep the data mapping the same (histogram[i] still maps to box[i+1])
        int stateLabel = i + 1;
        histBar.label.setFont(font);
        histBar.label.setCharacterSize(7);
        histBar.label.setFillColor(sf::Color::White);
        histBar.label.setString(std::to_string(stateLabel));

        // Center the label under each bar
        sf::FloatRect textBounds = histBar.label.getLocalBounds();
        float labelX = histX + i * barWidth + (barWidth - textBounds.width) / 2;
        histBar.label.setPosition(labelX, histY + histHeight + 5);

        histogram.push_back(histBar);
    }
}

void updateHistogram(std::vector<HistogramBar> &histogram, const std::vector<BoxInfo> &boxes)
{
    const float histY = 40.0f;
    const float histHeight = 180.0f;
    int maxVisits = 1;
    for (int i = 1; i < boxes.size(); ++i)
        maxVisits = std::max(maxVisits, boxes[i].visits);

    for (int i = 0; i < histogram.size(); ++i)
    {
        // Reverse the mapping: histogram bar i (labeled as state i+1) should show data from box (BOX_COUNT - 1 - i)
        int boxIndex = BOX_COUNT - 1 - i;
        int visits = boxes[boxIndex].visits;
        float barHeight = (visits > 0) ? (static_cast<float>(visits) / maxVisits) * histHeight : 0.0f;
        histogram[i].bar.setSize(sf::Vector2f(histogram[i].bar.getSize().x, barHeight));
        histogram[i].bar.setPosition(histogram[i].bar.getPosition().x, histY + histHeight - barHeight);

        histogram[i].valueText.setString(std::to_string(visits));
        sf::Vector2f barPos = histogram[i].bar.getPosition();

        // Center the value text above each bar
        sf::FloatRect textBounds = histogram[i].valueText.getLocalBounds();
        float textX = barPos.x + (histogram[i].bar.getSize().x - textBounds.width) / 2;
        float textY = barPos.y - 15;
        if (textY < histY)
            textY = barPos.y + 2;
        histogram[i].valueText.setPosition(textX, textY);
    }
}

sf::Color generateRandomColor()
{
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> colorDist(100, 255);
    return sf::Color(colorDist(rng), colorDist(rng), colorDist(rng));
}

std::vector<Ball> splitBall(const Ball &originalBall, float multiplicativeFactor,
                            const std::vector<BoxInfo> &boxes, std::mt19937 &rng)
{
    std::vector<Ball> newBalls;

    int wholePart = static_cast<int>(multiplicativeFactor);
    float fractionalPart = multiplicativeFactor - wholePart;

    int totalBalls = wholePart;

    // Handle fractional part with probability
    if (fractionalPart > 0)
    {
        std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
        if (probDist(rng) < fractionalPart)
        {
            totalBalls += 1;
        }
    }

    // Create the new balls
    for (int i = 0; i < totalBalls; ++i)
    {
        Ball newBall = originalBall;
        if (multiplicativeFactor > 1)
        {
            newBall.color = generateRandomColor();
        }
        else
        {
            newBall.color = sf::Color::Green;
        }
        newBall.trail.clear();
        newBall.hasReachedEnd = false;

        // Add slight position offset for visual separation
        float offsetX = (i - totalBalls / 2.0f) * 5.0f;
        newBall.position.x += offsetX;

        newBalls.push_back(newBall);
    }

    return newBalls;
}

void resetSimulation(std::vector<BoxInfo> &boxes, std::vector<HistogramBar> &histogram,
                     std::vector<Ball> &balls, int &stepCount, int &cycleCount, const std::vector<BoxInfo> &originalBoxes)
{
    // Reset all visit counts
    for (auto &box : boxes)
    {
        box.visits = 0;
        // box.countText.setString("0");
    }

    // Reset histogram
    updateHistogram(histogram, boxes);

    // Clear all balls and create initial ball
    balls.clear();
    sf::Vector2f initialPos;
    initialPos.x = originalBoxes[0].x * SCALE + BOX_WIDTH * SCALE / 2.f;
    initialPos.y = WINDOW_HEIGHT - originalBoxes[0].height * SCALE - BALL_RADIUS * SCALE;
    balls.push_back(Ball(initialPos, 0));

    // Reset counters
    stepCount = 0;
    cycleCount = 0;
}

void startNewCycle(std::vector<Ball> &balls, const std::vector<BoxInfo> &boxes, int &cycleCount)
{
    // Clear all balls and create new initial ball
    balls.clear();
    sf::Vector2f initialPos;
    initialPos.x = boxes[0].x * SCALE + BOX_WIDTH * SCALE / 2.f;
    initialPos.y = WINDOW_HEIGHT - boxes[0].height * SCALE - BALL_RADIUS * SCALE;
    balls.push_back(Ball(initialPos, 0));
    cycleCount++;

    // std::cout << "Starting cycle " << cycleCount << std::endl;
}

float getUserMultiplicativeFactor()
{
    float factor{1.0f}; // default
    std::cout << "Enter multiplicative factor (e.g., 2.0 for doubling, 1.5 for 50% chance of +1 ball): ";
    std::cin >> factor;

    if (factor < 1.0f)
        factor = 1.0f; // minimum bound
    if (factor > 3.0f)
        factor = 3.0f; // maximum bound

    std::cout << "Using multiplicative factor: " << factor << std::endl;
    return factor;
}

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
    // histTitle.setFont(font);
    // histTitle.setCharacterSize(14);
    // histTitle.setFillColor(sf::Color::White);
    // histTitle.setString("State Visit Frequency");
    // histTitle.setPosition(20, 5);

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

    // Create help panel background
    sf::RectangleShape helpPanel;
    helpPanel.setSize(sf::Vector2f(400, 160));
    helpPanel.setPosition(WINDOW_WIDTH - 420, 140);
    helpPanel.setFillColor(sf::Color(0, 0, 0, 150));
    helpPanel.setOutlineColor(sf::Color(100, 100, 100, 200));
    helpPanel.setOutlineThickness(1.0f);

    // Help panel title
    sf::Text helpTitle;
    helpTitle.setFont(font);
    helpTitle.setCharacterSize(14);
    helpTitle.setFillColor(sf::Color(255, 255, 255));
    helpTitle.setString("CONTROLS");
    helpTitle.setPosition(WINDOW_WIDTH - 400, 148);

    // Individual control labels
    // sf::Text resetHelp, screenshotHelp, speedUpHelp, slowDownHelp, factorHelp, pauseHelp;

    // resetHelp.setFont(font);
    // resetHelp.setCharacterSize(11);
    // resetHelp.setFillColor(sf::Color(200, 255, 200));
    // resetHelp.setString("R - Reset Simulation");
    // resetHelp.setPosition(WINDOW_WIDTH - 400, 168);

    // screenshotHelp.setFont(font);
    // screenshotHelp.setCharacterSize(11);
    // screenshotHelp.setFillColor(sf::Color(200, 200, 255));
    // screenshotHelp.setString("S - Save Screenshot");
    // screenshotHelp.setPosition(WINDOW_WIDTH - 400, 184);

    // speedUpHelp.setFont(font);
    // speedUpHelp.setCharacterSize(11);
    // speedUpHelp.setFillColor(sf::Color(255, 255, 200));
    // speedUpHelp.setString("+ - Speed Up");
    // speedUpHelp.setPosition(WINDOW_WIDTH - 400, 200);

    // slowDownHelp.setFont(font);
    // slowDownHelp.setCharacterSize(11);
    // slowDownHelp.setFillColor(sf::Color(255, 200, 200));
    // slowDownHelp.setString("- - Slow Down");
    // slowDownHelp.setPosition(WINDOW_WIDTH - 400, 216);

    // factorHelp.setFont(font);
    // factorHelp.setCharacterSize(11);
    // factorHelp.setFillColor(sf::Color(255, 200, 255));
    // factorHelp.setString("F - Change Factor");
    // factorHelp.setPosition(WINDOW_WIDTH - 400, 232);

    // pauseHelp.setFont(font);
    // pauseHelp.setCharacterSize(11);
    // pauseHelp.setFillColor(sf::Color(200, 255, 255));
    // pauseHelp.setString("SPACE - Pause/Resume");
    // pauseHelp.setPosition(WINDOW_WIDTH - 400, 248);

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

        // Draw help panel
        // window.draw(helpPanel);
        // window.draw(helpTitle);
        // window.draw(resetHelp);
        // window.draw(screenshotHelp);
        // window.draw(speedUpHelp);
        // window.draw(slowDownHelp);
        // window.draw(factorHelp);
        // window.draw(pauseHelp);
        // window.draw(speedLabel);
        // window.draw(pauseLabel);

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