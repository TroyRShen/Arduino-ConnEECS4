#include <gamma.h>
#include <RGBmatrixPanel.h>
#include <Adafruit_GFX.h>

// define the wiring of the LED screen
const uint8_t CLK  = 8;
const uint8_t LAT = A3;
const uint8_t OE = 9;
const uint8_t A = A0;
const uint8_t B = A1;
const uint8_t C = A2;

// a global variable that represents the LED screen
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);

// define the wiring of the inputs
const int POTENTIOMETER_PIN_NUMBER = 5;
const int BUTTON_PIN_NUMBER = 10;

//smoothing
const int NUM_READINGS = 10;

int readings[NUM_READINGS];  // the readings from the analog input
int readIndex = 0;          // the index of the current reading
int total = 0;              // the running total
int average = 0;            // the average

class Color {
  public:
    int red;
    int green;
    int blue;
    Color() {
      red = 0;
      green = 0;
      blue = 0;
    }
    Color(int r, int g, int b) {
      red = r;
      green = g;
      blue = b;
    }
    uint16_t to_333() const {
      return matrix.Color333(red, green, blue);
    }
};

const Color BLACK(0, 0, 0);
const Color RED(4, 0, 0);
const Color ORANGE(6, 1, 0);
const Color YELLOW(4, 4, 0);
const Color GREEN(0, 4, 0);
const Color BLUE(0, 0, 4);
const Color PURPLE(1, 0, 2);
const Color WHITE(4, 4, 4);
const Color LIME(2, 4, 0);
const Color AQUA(0, 4, 4);

const int BOARD_X = 32;
const int BOARD_Y = 16;

const int NUM_ROWS = 6;
const int NUM_COLS = 7;

class Player {
  public:
    //put here for convenience
    bool fired;
    Player() {
      //will always be odd - set in middle
      x = NUM_COLS - 1 + NUM_COLS % 2;
      //doesnt change unless button click
      y = BOARD_Y - NUM_ROWS * 2 - 2;
      color = false;
      fired = false;
    }

    //reset player to default
    void reset() {
      //will always be odd - set in middle
      x = NUM_COLS - 1 + NUM_COLS % 2;
      //starts 1 above grid
      y = BOARD_Y - NUM_ROWS * 2 - 2;
      color = false;
      fired = false;
    }

    //getters & setters
    int getX() {
      return x;
    }
    void setX(int xArg) {
      //make sure that player hasn't fired
      if (!fired) {
        x = xArg;
      }
    }
    int getY() {
      return y;
    }
    int setY(int yArg) {
      y = yArg;
    }
    char getColor() {
      if (color) {
        //blue
        return 'b';
      } else {
        //red
        return 'r';
      }
    }
    int toggleColor() {
      color = !color;
    }

    void move() {
      if (fired) {
        //erasing only certain pixel
        matrix.drawPixel(x, y, BLACK.to_333());
        y+=2;
        draw();
      }
    }

    //draw player
    void draw() {
      //figure out which color is chip
      Color currentColor = RED;
      if (color) {
        currentColor = BLUE;
      }
      matrix.drawPixel(x, y, currentColor.to_333());
    }

    void erase() {
      //just in case of glitch
      if (!fired) {
        matrix.drawPixel(x - 1, y, BLACK.to_333());
        matrix.drawPixel(x, y, BLACK.to_333());
        matrix.drawPixel(x + 1, y, BLACK.to_333());
      }
    }

  private:
    int x;
    int y;
    //false = red; true = blue;
    //toggle using ! feature
    bool color;
};

void colorInGrid() {
  // starting @(0,15) color in every other point white for grid (7 rows & 8 cols)
  //x is COLS y is ROWS
  for (int i = 0; i < NUM_COLS + 1; i++) {
    for (int j = 0; j < NUM_ROWS + 1; j++) {
      matrix.drawPixel(i * 2, BOARD_Y - j * 2 - 1, WHITE.to_333());
    }
  }
}

//print p1 or p2 depending on current move
void displayPlayerTurn(char color) {
  //reset certain area of screen
  matrix.fillRect(18, 5, 31, 15, BLACK.to_333());
  matrix.setCursor(19, 5);
  
  if (color == 'b') {
    matrix.setTextColor(BLUE.to_333());
    matrix.print("P2");
  } else {
    matrix.setTextColor(RED.to_333());
    matrix.print("P1");
  }
}

bool isWinner(char board[NUM_ROWS][NUM_COLS], char color) {
  for (int i = 0; i < NUM_ROWS; i++) {
    for (int j = 0; j < NUM_COLS; j++) {
      //first check if chip is same color as win
      Serial.print("0");
      Serial.print(" ");
      
      if (board[i][j] == color) {
        //need to first check for all checks if out of bounds
        //vertical check
        if (i + 3 < NUM_ROWS) {
          if (board[i][j] == board[i + 1][j] &&
          board[i + 1][j] == board[i + 2][j] &&
          board[i + 2][j] == board[i + 3][j]) {
            return true;
          }
        }

        //horizontal check
        if (j + 3 < NUM_COLS) {
          if (board[i][j] == board[i][j + 1] &&
          board[i][j + 1] == board[i][j + 2] &&
          board[i][j + 2] == board[i][j + 3]) {
            return true;
          }
        }

        //left diagonal check
        if (i + 3 < NUM_ROWS && j - 3 >= 0) {
          if (board[i][j] == board[i + 1][j - 1] &&
          board[i + 1][j - 1] == board[i + 2][j - 2] &&
          board[i + 2][j - 2] == board[i + 3][j - 3]) {
            return true;
          }
        }


        //right diagonal check
        if (i + 3 < NUM_ROWS && j + 3 < NUM_COLS) {
          if (board[i][j] == board[i + 1][j + 1] &&
          board[i + 1][j + 1] == board[i + 2][j + 2] &&
          board[i + 2][j + 2] == board[i + 3][j + 3]) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

void displayWinner(char color) {
  matrix.fillScreen(BLACK.to_333());
  matrix.setCursor(1, 1);
  if (color == 'b') {
    matrix.setTextColor(BLUE.to_333());
    matrix.print("P2");
    matrix.setCursor(1, 9);
    matrix.print("Wins!");
  } else if (color == 'r') {
    matrix.setTextColor(RED.to_333());
    matrix.print("P1");
    matrix.setCursor(1, 9);
    matrix.print("Wins!");
  } else {
    matrix.setTextColor(WHITE.to_333());
    matrix.print("Tie");
    matrix.setCursor(1, 9);
    matrix.print("Game");
  }
}

class Game {
  public:
    void setupGame() {
      //reset screen
      matrix.fillScreen(BLACK.to_333());
      player.reset();
      
      //reset counter
      totalChipsDropped = 0;

      //intialize board
      for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_COLS; j++) {
          board[i][j] = 'n';
        }
      }
      displayPlayerTurn(player.getColor());
      colorInGrid();
    }

    void update(int potentiometer_value, bool button_pressed) {
      time = millis();
      bool hasWon = false;

      //7 total positions for player to be
      int potentiometerPos = potentiometer_value / 157;

      //movement - lock movement if dropping
      if (!player.fired) {
        player.erase();
        player.setX(potentiometerPos * 2 + 1);
        player.draw();
      }

      //check if chip shot
      if (button_pressed) {
        //locking mecahnism w/ if statement
        if (!player.fired) {
          droppedColPos = potentiometerPos;
        }
        player.fired = true;
      }

      if (player.fired) {
        //check for collision
        // if chip two below or bottom
        if (board[droppedRowPos][droppedColPos] != 'n'
          || droppedRowPos == 6) {
          //set chip color there to respective color depending on situation
          if (droppedRowPos != 0) {
            //drop chip
            board[droppedRowPos - 1][droppedColPos] = player.getColor();
            totalChipsDropped++;

            //check if whey won
            hasWon = isWinner(board, player.getColor());

            //then toggle color & change player sign
            player.toggleColor();
            displayPlayerTurn(player.getColor());
          }

          //reset player
          droppedRowPos = 0;
          droppedColPos = 0;
          player.setY(BOARD_Y - NUM_ROWS * 2 - 2);
          player.fired = false;

          //avoid chip spam if they haven't won
          if (!hasWon) {
            delay(200);
          }

        } else if (time % 50 == 0) {
          player.move();
          droppedRowPos++;
        }
      }

      //check if any1 has won
      if (hasWon) {
        //toggle player color back bc of chip drop
        player.toggleColor();
        //display winner for 1 sec & reset game
        displayWinner(player.getColor());
        delay(2000);
        setupGame();
      } else if (totalChipsDropped == NUM_ROWS * NUM_COLS) {
        displayWinner('n');
        delay(2000);
        setupGame();
      }
    }
  private:
    int previousPotentiometerVal = 0;
    //for checking if any chips below dropped one
    int droppedColPos = 0;
    int droppedRowPos = 0;
    //for checking if all chips dropped
    int totalChipsDropped = 0;
    unsigned long time = 0;
    Player player;
    // colors: r = red; b = blue; n = none
    char board[NUM_ROWS][NUM_COLS];
};

//global variable that represents ConnEECS4 game
Game game;

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN_NUMBER, INPUT);

  //set intial values
  for (int i = 0; i < NUM_READINGS; i++) {
    readings[i] = 0;
  }

  matrix.begin();
  game.setupGame();
}

// see https://www.arduino.cc/reference/en/language/structure/sketch/loop/
void loop() {
  //more smoothing
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(POTENTIOMETER_PIN_NUMBER);
  total = total + readings[readIndex];
  readIndex = readIndex + 1;

  //wrap if at end
  if (readIndex >= NUM_READINGS) {
    readIndex = 0;
  }
  
  //average is reading + delay for more smoothing
  int potentiometer_value = total / NUM_READINGS;
  delay(1);

  bool button_pressed = (digitalRead(BUTTON_PIN_NUMBER) == HIGH);

  game.update(potentiometer_value, button_pressed);
}
