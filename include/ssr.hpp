#pragma once 
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

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
    if (factor > 4.0f)
        factor = 4.0f; // maximum bound

    std::cout << "Using multiplicative factor: " << factor << std::endl;
    return factor;
}