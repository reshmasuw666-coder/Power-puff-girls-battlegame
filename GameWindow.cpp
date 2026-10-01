#include "GameWindow.h"

#include <algorithm>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QWidget>

namespace {

// Qt key -> SFML key (only the keys the game uses)
sf::Keyboard::Key toSfKey(int k)
{
    using K = sf::Keyboard;
    switch (k) {
    case Qt::Key_W:      return K::W;
    case Qt::Key_A:      return K::A;
    case Qt::Key_S:      return K::S;
    case Qt::Key_D:      return K::D;
    case Qt::Key_P:      return K::P;
    case Qt::Key_M:      return K::M;
    case Qt::Key_R:      return K::R;
    case Qt::Key_Up:     return K::Up;
    case Qt::Key_Down:   return K::Down;
    case Qt::Key_Left:   return K::Left;
    case Qt::Key_Right:  return K::Right;
    case Qt::Key_Space:  return K::Space;
    case Qt::Key_Return:
    case Qt::Key_Enter:  return K::Return;
    case Qt::Key_Escape: return K::Escape;
    case Qt::Key_Shift:  return K::LShift;
    default:             return K::Unknown;
    }
}

} // namespace

GameWindow::GameWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Power Puff Girls - Battle");
    resize(1000, 650);

    auto* container = new QWidget(this);
    container->setAttribute(Qt::WA_NativeWindow);
    container->setAttribute(Qt::WA_PaintOnScreen);
    container->setAttribute(Qt::WA_OpaquePaintEvent);
    container->setAttribute(Qt::WA_NoSystemBackground);
    container->setFocusPolicy(Qt::StrongFocus);
    setCentralWidget(container);

    connect(&timer, &QTimer::timeout, this, &GameWindow::updateGame);

    setFocusPolicy(Qt::StrongFocus);
    container->setFocus();
}

void GameWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    if (!sfmlStarted) {
        sfmlStarted = true;
        startSfml();
        timer.start(16);   // about 60 FPS
    }
}

GameWindow::~GameWindow()
{
    timer.stop();

    if (sfmlWindow) {
        sfmlWindow->close();
        delete sfmlWindow;
        sfmlWindow = nullptr;
    }
}

void GameWindow::startSfml()
{
    QWidget* container = centralWidget();

    sfmlWindow = new sf::RenderWindow();
    sfmlWindow->create(reinterpret_cast<sf::WindowHandle>(container->winId()));
    sfmlWindow->setVerticalSyncEnabled(true);
    clock.restart();
}

void GameWindow::updateGame()
{
    if (!sfmlWindow)
        return;

    sf::Event event;
    while (sfmlWindow->pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            close();
            return;
        }
    }

    const float dt = clock.restart().asSeconds();

    gameManager.update(dt, sfmlWindow->getSize());
    paintGame();
}

void GameWindow::paintGame()
{
    if (!sfmlWindow)
        return;

    sfmlWindow->clear(sf::Color(30, 18, 34));   // colour of the letterbox bars
    gameManager.render(*sfmlWindow);
    sfmlWindow->display();
}

void GameWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
}

void GameWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat())
        return;

    const sf::Keyboard::Key k = toSfKey(event->key());
    if (k != sf::Keyboard::Unknown)
        gameManager.handleKeyPressed(k);
    else
        QMainWindow::keyPressEvent(event);
}

void GameWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat())
        return;

    const sf::Keyboard::Key k = toSfKey(event->key());
    if (k != sf::Keyboard::Unknown)
        gameManager.handleKeyReleased(k);
    else
        QMainWindow::keyReleaseEvent(event);
}

void GameWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && sfmlWindow) {
        QWidget* c = centralWidget();
        const QPoint p = c->mapFrom(this, event->position().toPoint());
        // Qt works in logical pixels, SFML in real pixels (differs on high-DPI screens)
        const float sx = static_cast<float>(sfmlWindow->getSize().x) / static_cast<float>(std::max(1, c->width()));
        const float sy = static_cast<float>(sfmlWindow->getSize().y) / static_cast<float>(std::max(1, c->height()));
        gameManager.handleMousePressed({static_cast<float>(p.x()) * sx, static_cast<float>(p.y()) * sy});
    }

    QMainWindow::mousePressEvent(event);
}

void GameWindow::closeEvent(QCloseEvent* event)
{
    if (sfmlWindow)
        sfmlWindow->close();

    QMainWindow::closeEvent(event);
}
