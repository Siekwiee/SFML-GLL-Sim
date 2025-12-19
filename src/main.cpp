#include <SFML/Graphics.hpp>
#include <cstdio>
#include <iostream>
#include <vector>
#include "Parser.hpp"
#include "Sim.hpp"
#include "UI.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: GLLSimulator <program.txt>\n");
    return 1;
  }

  auto prog = std::make_unique<Program>();
  const char* fPath = argv[1];
  prog->lastModifiedAt = std::filesystem::last_write_time(fPath);
  auto res = parseFile(argv[1], *prog);
  if (!res.ok) {
    fprintf(stderr, "Parse error: %s\n", res.msg.c_str());
    return 1;
  }
  auto sim = std::make_unique<Simulator>(*prog);
  // Create fullscreen window (borderless)
  auto fsModes = sf::VideoMode::getFullscreenModes();
  sf::RenderWindow win(fsModes[0], "GLLSimulator", sf::Style::Default);
  win.setFramerateLimit(60);
  win.setTitle("GLL - " + std::filesystem::path(fPath).filename().string());
  auto ui = std::make_unique<UI>(*prog, *sim);
  ui->updateLayout(win.getSize());

  sf::Clock clock;
  while (win.isOpen()) {
    if (fileWatcher(fPath, *prog)) {
      Program tmpProg;
      if (parseFile(fPath, tmpProg).ok) {
        // Successfully parsed new changes - hot reloading now
        tmpProg.lastModifiedAt = std::filesystem::last_write_time(fPath);
        *prog = std::move(tmpProg);
        // recreate sim and ui to make sure sizes match
        sim = std::make_unique<Simulator>(*prog);
        ui = std::make_unique<UI>(*prog, *sim);

        ui->updateLayout(win.getSize());
        win.setTitle("GLL - " + std::filesystem::path(fPath).filename().string());
        printf("Hot-Reload complete\n");
      }
      Simulator sim(tmpProg);
      ui->updateLayout(win.getSize());
    }
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
      ui->handleEvent(win, *event);
    }

    float dt = clock.restart().asSeconds();
    ui->update(dt);
    sim->update(dt, ui->simSpeed(), ui->isRunning(), ui->stepOnceRequested());

    win.clear(Theme::Background);
    ui->draw(win);
    win.display();
  }

  return 0;
}
