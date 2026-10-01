These files add the missing GameWindow/GameManager classes expected by main.cpp.

Important:
- This version uses SFML 2.x style APIs.
- Your CMakeLists.txt must link Qt Widgets and SFML Graphics/Window/System.
- Add GameWindow.cpp, GameManager.cpp, GameWindow.h and GameManager.h to the CMake target.
- If your installed SFML is version 3.x, these source files need API changes.
- Do not delete your existing Character files until the project builds successfully.
