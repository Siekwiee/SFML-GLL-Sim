#include <SFML/Graphics.hpp>
#include <cstdio>
#include <iostream>
#include <vector>
#include "Parser.hpp"
#include "Sim.hpp"
#include "UI.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: Gates <program.txt>\n");
    return 1;
  }

  Program prog;
  auto res = parseFile(argv[1], prog);
  if (!res.ok) {
    fprintf(stderr, "Parse error: %s\n", res.msg.c_str());
    return 1;
  }

  Simulator sim(prog);
  
  // Create fullscreen window (borderless)
  auto fsModes = sf::VideoMode::getFullscreenModes();
  sf::RenderWindow win(fsModes[0], "Gates", sf::Style::Default);
  win.setFramerateLimit(60);
  
  UI ui(prog, sim);
  ui.updateLayout(win.getSize());

  sf::Clock clock;
  while (win.isOpen()) {
    for (auto event = win.pollEvent(); event.has_value(); event = win.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        win.close();
      }
      // Escape key to exit fullscreen
      if (auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Escape) {
          win.close();
        }
      }
      ui.handleEvent(win, *event);
    }

    float dt = clock.restart().asSeconds();
    ui.update(dt);
    sim.update(dt, ui.simSpeed(), ui.isRunning(), ui.stepOnceRequested());

    win.clear(Theme::Background);
    ui.draw(win);
    win.display();
  }

  return 0;
}
