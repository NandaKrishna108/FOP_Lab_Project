#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#define PORT 8080
#define BUFFER_SIZE 4096
#define WORD_COUNT 50
#define CLUE_COUNT 3
#define MAX_ATTEMPTS 5

// Game state structure
typedef struct {
    char baseWord[20];
    char numbers[3];
    char special[2];
    char password[40];
    int wordIndex;
    int currentAttempt;
    int numberClueStage;
    int gameOver;
    int gameWon;
} GameState;

GameState gameState;

const char *words[WORD_COUNT] = {
    "dragon","wizard","castle","phoenix","unicorn",
    "shadow","mirror","thunder","forest","moonlight",
    "griffin","mermaid","knight","spellbook","crown",
    "vampire","werewolf","ghost","potion","oracle",
    "harry","hermione","ron","hogwarts","dobby",
    "pikachu","naruto","goku","elsa","totoro",
    "narnia","aslan","hobbit","frodo","gandalf",
    "sword","shield","helmet","torch","map",
    "clock","keyhole","window","lantern","river"
};

const char *clues[WORD_COUNT][CLUE_COUNT] = {
    {"Breathes fire","Mythical creature","Guards treasure"},
    {"Uses magic","Wears robes","Learns spells"},
    {"Stone building","Has towers","Kings lived here"},
    {"Rises from ashes","Fire bird","Rebirth symbol"},
    {"Has a horn","Magical horse","Pure creature"},
    {"Dark shape","Follows you","No light"},
    {"Shows reflection","Glass object","You look into it"},
    {"Loud sound","With lightning","Storm related"},
    {"Many trees","Natural place","Animals live here"},
    {"Soft glow","From the moon","Night light"},
    {"Lion eagle mix","Mythical beast","Has wings"},
    {"Half fish","Lives in sea","Sings"},
    {"Wears armor","Uses sword","Protects kingdom"},
    {"Magic book","Old pages","Spells inside"},
    {"Royal headwear","Symbol of power","Kings wear it"},
    {"Drinks blood","Avoids sun","Undead"},
    {"Man beast","Full moon","Howls"},
    {"Transparent","Haunts places","Scary"},
    {"Magic liquid","Used in spells","Potion"},
    {"Sees future","Wise figure","Prophecy"},
    {"Boy wizard","Scar","Chosen one"},
    {"Smart witch","Book lover","Top student"},
    {"Red hair","Best friend","Chess"},
    {"Magic school","Houses","Hidden castle"},
    {"House elf","Big ears","Loves socks"},
    {"Yellow mouse","Electric","Pokemon"},
    {"Ninja boy","Orange clothes","Dreams big"},
    {"Strong fighter","Eats a lot","Saiyan"},
    {"Ice powers","Snow queen","Frozen"},
    {"Forest spirit","Fluffy","Ghibli"},
    {"Fantasy land","Wardrobe","Talking animals"},
    {"Lion king","Strong roar","Good ruler"},
    {"Small creature","Likes comfort","Shire"},
    {"Ring bearer","Brave","Resists power"},
    {"Wise wizard","Long beard","Grey robes"},
    {"Sharp weapon","Battle use","Handheld"},
    {"Defense gear","Blocks attacks","Warrior tool"},
    {"Head armor","Metal","Protection"},
    {"Fire light","Held at night","Torch"},
    {"Navigation tool","Paper","Travel"},
    {"Shows time","Ticks","Has hands"},
    {"Door opening","For keys","Lock"},
    {"Lets light in","Glass","Wall part"},
    {"Portable light","Outdoor","Lantern"},
    {"Flowing water","Natural","Moves"}
};

const char *rageMessages[] = {
    "Wrong. The password is disappointed.",
    "Incorrect. Confidence was not enough.",
    "That guess had ambition. Not accuracy.",
    "Still wrong. Try thinking this time.",
    "Impressive commitment to being incorrect.",
    "Nope. That was painful to watch."
};

void initGame() {
    gameState.numberClueStage=0;
    gameState.wordIndex = rand() % WORD_COUNT;
    strcpy(gameState.baseWord, words[gameState.wordIndex]);
    
    sprintf(gameState.numbers, "%d%d", rand() % 10, rand() % 10);
    
    char specialChars[] = "!@#$&_^+*";
    gameState.special[0] = specialChars[rand() % strlen(specialChars)];
    gameState.special[1] = '\0';
    
    sprintf(gameState.password, "%s%s%s", gameState.baseWord, gameState.numbers, gameState.special);
    
    gameState.currentAttempt = 0;
    gameState.gameOver = 0;
    gameState.gameWon = 0;
    
    printf("New game started. Password: %s\n", gameState.password);
}

void getSpecialClue(char sc, char *clueBuffer) {
    switch (sc) {
        case '$': 
            strcpy(clueBuffer, "The one that you always longed for.");
            break;
        case '#': 
            strcpy(clueBuffer, "Ever played tic tac toe?");
            break;
        case '@': 
            strcpy(clueBuffer, "You have typed this more in emails than in words.");
            break;
        case '!': 
            strcpy(clueBuffer, "Used when calm sentences are not enough.");
            break;
        case '&':
            strcpy(clueBuffer, "Means 'and', but fancier.");
            break;
        case '_': 
            strcpy(clueBuffer, "The space that is not a space.");
            break;
        case '+': 
            strcpy(clueBuffer, "Adds to the chaos.");
            break;
        case '*': 
            strcpy(clueBuffer, "A wildcard.");
            break;
        case '^': 
            strcpy(clueBuffer, "Goes up. Always.");
            break;
        default: 
            strcpy(clueBuffer, "A mysterious symbol.");
    }
}

void getNumberClue(int d1, int d2, int stage, char *clueBuffer) {
    if (stage == 0) {
        sprintf(clueBuffer, "The sum of the two digits is %d.", d1 + d2);
    }
    else if (stage == 1) {
        if (d1 % 2 == 0 && d2 % 2 == 0)
            strcpy(clueBuffer, "Both digits are even.");
        else if (d1 % 2 != 0 && d2 % 2 != 0)
            strcpy(clueBuffer, "Both digits are odd.");
        else
            strcpy(clueBuffer, "One digit is even and the other is odd.");
    }
    else {
        strcpy(clueBuffer, "No more number clues.");
    }
}
void sendHTTPResponse(int client_socket, const char *status, const char *content_type, const char *body) {
    char response[BUFFER_SIZE];
    int body_length = strlen(body);
    
    sprintf(response, 
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status, content_type, body_length, body);
    
    send(client_socket, response, strlen(response), 0);
}

void handleNewGame(int client_socket) {
    initGame();
    
    char json[1024];
    sprintf(json, 
        "{\"success\":true,\"message\":\"New game started\",\"attemptsRemaining\":%d}",
        MAX_ATTEMPTS);
    
    sendHTTPResponse(client_socket, "200 OK", "application/json", json);
}

void handleGetClue(int client_socket) {
    char json[512];
    
    if (gameState.currentAttempt < CLUE_COUNT) {
        sprintf(json, 
            "{\"success\":true,\"clue\":\"%s\",\"clueNumber\":%d}",
            clues[gameState.wordIndex][gameState.currentAttempt],
            gameState.currentAttempt + 1);
    } else {
        sprintf(json, 
            "{\"success\":true,\"clue\":\"All word clues shown\",\"clueNumber\":%d}",
            gameState.currentAttempt + 1);
    }
    
    sendHTTPResponse(client_socket, "200 OK", "application/json", json);
}

void handleGuess(int client_socket, const char *guess) {
    if (gameState.gameOver || gameState.gameWon) {
        char json[256];
        sprintf(json, "{\"success\":false,\"message\":\"Game is already over\"}");
        sendHTTPResponse(client_socket, "200 OK", "application/json", json);
        return;
    }
    
    // Parse guess
    char guessWord[20] = {0};
    char guessNumbers[3] = {0};
    char guessSpecial[2] = {0};
    
    int i = 0, j = 0;
    while (guess[i] && isalpha(guess[i])) {
        guessWord[j++] = tolower(guess[i++]);
    }
    guessWord[j] = '\0';
    
    if (guess[i]) guessNumbers[0] = guess[i++];
    if (guess[i]) guessNumbers[1] = guess[i++];
    guessNumbers[2] = '\0';
    
    if (strlen(guess) > 0) {
        guessSpecial[0] = guess[strlen(guess) - 1];
        guessSpecial[1] = '\0';
    }
    
    // Check matches
    int wordMatch = (strcasecmp(guessWord, gameState.baseWord) == 0);
    int numberMatch = (strcmp(guessNumbers, gameState.numbers) == 0);
    int specialMatch = (strcmp(guessSpecial, gameState.special) == 0);
    
    char json[2048];
    
    // Check for win
    if (wordMatch && numberMatch && specialMatch) {
        gameState.gameWon = 1;
        gameState.gameOver = 1;
        
        sprintf(json, 
            "{\"success\":true,\"gameWon\":true,\"message\":\"Access Granted!\",\"password\":\"%s\",\"attempts\":%d}",
            gameState.password, gameState.currentAttempt + 1);
        
        sendHTTPResponse(client_socket, "200 OK", "application/json", json);
        return;
    }
    
    // Generate clues
    char numberClue[256] = "";
    char specialClue[256] = "";
    
    int d1 = gameState.numbers[0] - '0';
    int d2 = gameState.numbers[1] - '0';
    
    if (wordMatch && !numberMatch) {
    getNumberClue(d1, d2, gameState.numberClueStage, numberClue);
    gameState.numberClueStage++;   
}
    
    if (wordMatch && !specialMatch) {
        getSpecialClue(gameState.special[0], specialClue);
    }
    
    // Get rage message
    const char *rageMsg = rageMessages[rand() % 6];
    
    gameState.currentAttempt++;
    
    // Check if game over
    if (gameState.currentAttempt >= MAX_ATTEMPTS) {
        gameState.gameOver = 1;
        
        sprintf(json, 
            "{\"success\":true,\"gameOver\":true,\"message\":\"%s\",\"password\":\"%s\",\"numberClue\":\"%s\",\"specialClue\":\"%s\"}",
            rageMsg, gameState.password, numberClue, specialClue);
    } else {
        sprintf(json, 
            "{\"success\":true,\"gameOver\":false,\"message\":\"%s\",\"attemptsRemaining\":%d,\"numberClue\":\"%s\",\"specialClue\":\"%s\",\"wordMatch\":%s,\"numberMatch\":%s,\"specialMatch\":%s}",
            rageMsg, MAX_ATTEMPTS - gameState.currentAttempt, numberClue, specialClue,
            wordMatch ? "true" : "false", numberMatch ? "true" : "false", specialMatch ? "true" : "false");
    }
    
    sendHTTPResponse(client_socket, "200 OK", "application/json", json);
}

void handleRequest(int client_socket, const char *request) {
    char method[16], path[256], body[1024];
    sscanf(request, "%s %s", method, path);
    
    // Handle CORS preflight
    if (strcmp(method, "OPTIONS") == 0) {
        sendHTTPResponse(client_socket, "200 OK", "text/plain", "");
        return;
    }
    
    // Extract body for POST requests
    char *body_start = strstr(request, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        strcpy(body, body_start);
    } else {
        body[0] = '\0';
    }
    
    printf("Request: %s %s\n", method, path);
    
    if (strcmp(path, "/api/newgame") == 0) {
        handleNewGame(client_socket);
    }
    else if (strcmp(path, "/api/getclue") == 0) {
        handleGetClue(client_socket);
    }
    else if (strcmp(path, "/api/guess") == 0 && strcmp(method, "POST") == 0) {
        // Parse JSON body to get guess
        char *guessStart = strstr(body, "\"guess\":\"");
        if (guessStart) {
            guessStart += 9; // Skip "guess":"
            char guess[100] = {0};
            int i = 0;
            while (guessStart[i] && guessStart[i] != '"' && i < 99) {
                guess[i] = guessStart[i];
                i++;
            }
            guess[i] = '\0';
            handleGuess(client_socket, guess);
        } else {
            sendHTTPResponse(client_socket, "400 Bad Request", "application/json", 
                "{\"success\":false,\"message\":\"Invalid request\"}");
        }
    }
    else {
        sendHTTPResponse(client_socket, "404 Not Found", "application/json", 
            "{\"success\":false,\"message\":\"Endpoint not found\"}");
    }
}

int main() {
    srand(time(NULL));
    
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif
    
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("Socket creation failed\n");
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    
    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Bind failed\n");
        close(server_socket);
        return 1;
    }
    
    // Listen
    if (listen(server_socket, 5) < 0) {
        printf("Listen failed\n");
        close(server_socket);
        return 1;
    }
    
    printf("Server running on http://localhost:%d\n", PORT);
    printf("Waiting for connections...\n\n");
    
    // Initialize first game
    initGame();
    
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            printf("Accept failed\n");
            continue;
        }
        
        memset(buffer, 0, BUFFER_SIZE);
        recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        handleRequest(client_socket, buffer);
        
        close(client_socket);
    }
    
    close(server_socket);
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}
