#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <curl/curl.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libcurl.lib")

#define PORT 7000

// Configuration - replace with your actual credentials
const std::string FACEBOOK_APP_ID = "YOUR_FACEBOOK_APP_ID";
const std::string FACEBOOK_APP_SECRET = "YOUR_FACEBOOK_APP_SECRET";
const std::string TWITTER_API_KEY = "YOUR_TWITTER_API_KEY";
const std::string TWITTER_API_SECRET = "YOUR_TWITTER_API_SECRET";
const std::string SERVER_URL = "http://localhost:7000"; // Change to your public URL

struct Post {
    int id;
    std::string title;
    std::string content;
    std::string author;
    std::time_t timestamp;
    std::map<std::string, std::string> social_media_ids; // Stores post IDs from social platforms
};

struct UserSession {
    std::string id;
    std::string platform;
    std::string access_token;
    std::time_t expires_at;
};

std::vector<Post> posts;
std::map<std::string, UserSession> sessions;
int nextPostId = 1;

// Callback for CURL writes
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Helper function to URL encode strings
std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        }
        else {
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }

    return escaped.str();
}

// Helper function to decode URL encoded strings
std::string urlDecode(const std::string& str) {
    std::string result;
    char ch;
    int i, ii;
    
    for (i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            result += ch;
            i = i + 2;
        }
        else if (str[i] == '+') {
            result += ' ';
        }
        else {
            result += str[i];
        }
    }
    return result;
}

// Generate HTML responses
std::string generateHTMLResponse(const std::string& content) {
    std::string html = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    html += "<!DOCTYPE html><html><head><title>Post Server</title>";
    html += "<style>body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }";
    html += "textarea { width: 100%; height: 100px; }</style></head><body>";
    html += content;
    html += "</body></html>";
    return html;
}

// Generate JSON responses
std::string generateJSONResponse(const std::string& json) {
    return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json;
}

// Get all posts as HTML
std::string getAllPostsHTML() {
    std::string content = "<h1>All Posts</h1>";
    content += "<a href='/create'>Create New Post</a> | ";
    content += "<a href='/login/facebook'>Login with Facebook</a> | ";
    content += "<a href='/login/twitter'>Login with Twitter</a><br><br>";
    
    for (const auto& post : posts) {
        content += "<div style='border: 1px solid #ccc; padding: 10px; margin-bottom: 15px;'>";
        content += "<h2>" + post.title + "</h2>";
        content += "<p>By: " + post.author + "</p>";
        
        // Format timestamp
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&post.timestamp));
        content += "<small>Posted on: " + std::string(buffer) + "</small><br><br>";
        
        content += "<p>" + post.content + "</p>";
        content += "<div style='margin-top: 10px;'>";
        content += "<a href='/edit/" + std::to_string(post.id) + "'>Edit</a> | ";
        content += "<a href='/delete/" + std::to_string(post.id) + "'>Delete</a> | ";
        
        // Sharing buttons
        std::string postUrl = SERVER_URL + "/post/" + std::to_string(post.id);
        std::string shareText = urlEncode(post.title + ": " + post.content.substr(0, 50) + "...");
        
        content += "<a href='https://twitter.com/intent/tweet?text=" + shareText + "&url=" + urlEncode(postUrl) + "' target='_blank'>Share on Twitter</a> | ";
        content += "<a href='https://www.facebook.com/sharer/sharer.php?u=" + urlEncode(postUrl) + "' target='_blank'>Share on Facebook</a>";
        content += "</div></div>";
    }
    
    return generateHTMLResponse(content);
}

// Get create post form
std::string getCreatePostForm(const std::string& author = "") {
    std::string form = "<h1>Create New Post</h1>";
    form += "<form method='POST' action='/create'>";
    form += "Title: <input type='text' name='title' required><br>";
    form += "Author: <input type='text' name='author' value='" + author + "' required><br>";
    form += "Content: <textarea name='content' required></textarea><br>";
    form += "<input type='checkbox' name='share_facebook' value='1'> Share to Facebook<br>";
    form += "<input type='checkbox' name='share_twitter' value='1'> Share to Twitter<br>";
    form += "<input type='submit' value='Create Post'>";
    form += "</form>";
    form += "<a href='/'>Back to all posts</a>";
    
    return generateHTMLResponse(form);
}

// Post to Facebook
bool postToFacebook(const Post& post, const std::string& accessToken) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string readBuffer;
        std::string postUrl = "https://graph.facebook.com/me/feed";
        
        std::string postData = "message=" + urlEncode(post.title + "\n\n" + post.content) +
                              "&access_token=" + accessToken;
        
        curl_easy_setopt(curl, CURLOPT_URL, postUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK) {
            // Parse response to get Facebook post ID
            size_t idPos = readBuffer.find("\"id\":\"");
            if (idPos != std::string::npos) {
                size_t endPos = readBuffer.find("\"", idPos + 6);
                std::string fbPostId = readBuffer.substr(idPos + 6, endPos - (idPos + 6));
                // Store this ID in your post's social_media_ids map
                return true;
            }
        }
    }
    return false;
}

// Post to Twitter
bool postToTwitter(const Post& post, const std::string& accessToken) {
    // Similar implementation to Facebook but using Twitter API
    // Note: Twitter API v2 requires OAuth 1.0a signing which is more complex
    // This is a simplified placeholder
    return false;
}

// Handle creating a new post
void handleCreatePost(const std::string& request, SOCKET clientSocket) {
    size_t contentPos = request.find("\r\n\r\n");
    if (contentPos != std::string::npos) {
        std::string postData = request.substr(contentPos + 4);
        
        // Parse form data
        std::map<std::string, std::string> params;
        size_t pos = 0;
        while (pos < postData.length()) {
            size_t ampPos = postData.find('&', pos);
            if (ampPos == std::string::npos) ampPos = postData.length();
            
            std::string pair = postData.substr(pos, ampPos - pos);
            size_t equalPos = pair.find('=');
            if (equalPos != std::string::npos) {
                std::string key = urlDecode(pair.substr(0, equalPos));
                std::string value = urlDecode(pair.substr(equalPos + 1));
                params[key] = value;
            }
            
            pos = ampPos + 1;
        }
        
        // Create new post
        Post newPost;
        newPost.id = nextPostId++;
        newPost.title = params["title"];
        newPost.author = params["author"];
        newPost.content = params["content"];
        newPost.timestamp = std::time(nullptr);
        posts.push_back(newPost);
        
        // Check if we should share to social media
        std::string sessionCookie;
        size_t cookiePos = request.find("Cookie: ");
        if (cookiePos != std::string::npos) {
            size_t endPos = request.find("\r\n", cookiePos);
            sessionCookie = request.substr(cookiePos + 8, endPos - (cookiePos + 8));
        }
        
        if (params.count("share_facebook") && sessions.count(sessionCookie)) {
            UserSession& session = sessions[sessionCookie];
            if (session.platform == "facebook") {
                if (postToFacebook(newPost, session.access_token)) {
                    // Successfully posted to Facebook
                }
            }
        }
        
        if (params.count("share_twitter") && sessions.count(sessionCookie)) {
            UserSession& session = sessions[sessionCookie];
            if (session.platform == "twitter") {
                if (postToTwitter(newPost, session.access_token)) {
                    // Successfully posted to Twitter
                }
            }
        }
        
        // Redirect to home page
        std::string response = "HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n";
        send(clientSocket, response.c_str(), response.length(), 0);
        return;
    }
    
    // If we get here, something went wrong
    std::string response = generateHTMLResponse("<h1>Error</h1><p>Failed to create post</p>");
    send(clientSocket, response.c_str(), response.length(), 0);
}

// Handle Facebook login redirect
std::string handleFacebookLogin() {
    std::string redirectUrl = "https://www.facebook.com/v12.0/dialog/oauth?client_id=" + FACEBOOK_APP_ID +
                             "&redirect_uri=" + urlEncode(SERVER_URL + "/auth/facebook") +
                             "&scope=public_profile,email,publish_to_groups,publish_pages";
    
    return "HTTP/1.1 302 Found\r\nLocation: " + redirectUrl + "\r\n\r\n";
}

// Handle Facebook auth callback
std::string handleFacebookAuth(const std::string& code) {
    // Exchange code for access token
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string readBuffer;
        std::string tokenUrl = "https://graph.facebook.com/v12.0/oauth/access_token?" +
                               std::string("client_id=") + FACEBOOK_APP_ID +
                               "&redirect_uri=" + urlEncode(SERVER_URL + "/auth/facebook") +
                               "&client_secret=" + FACEBOOK_APP_SECRET +
                               "&code=" + code;
        
        curl_easy_setopt(curl, CURLOPT_URL, tokenUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res == CURLE_OK) {
            // Parse access token from response
            size_t tokenPos = readBuffer.find("\"access_token\":\"");
            if (tokenPos != std::string::npos) {
                size_t endPos = readBuffer.find("\"", tokenPos + 16);
                std::string accessToken = readBuffer.substr(tokenPos + 16, endPos - (tokenPos + 16));
                
                // Get user info
                curl = curl_easy_init();
                if (curl) {
                    readBuffer.clear();
                    std::string userUrl = "https://graph.facebook.com/me?fields=id,name&access_token=" + accessToken;
                    
                    curl_easy_setopt(curl, CURLOPT_URL, userUrl.c_str());
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
                    
                    res = curl_easy_perform(curl);
                    curl_easy_cleanup(curl);
                    
                    if (res == CURLE_OK) {
                        size_t namePos = readBuffer.find("\"name\":\"");
                        if (namePos != std::string::npos) {
                            size_t nameEndPos = readBuffer.find("\"", namePos + 8);
                            std::string userName = readBuffer.substr(namePos + 8, nameEndPos - (namePos + 8));
                            
                            // Create session
                            UserSession session;
                            session.id = "sess_" + std::to_string(std::time(nullptr));
                            session.platform = "facebook";
                            session.access_token = accessToken;
                            session.expires_at = std::time(nullptr) + 3600; // 1 hour expiration
                            
                            sessions[session.id] = session;
                            
                            // Set cookie and redirect
                            std::string response = "HTTP/1.1 302 Found\r\n";
                            response += "Set-Cookie: session=" + session.id + "; Path=/; Max-Age=3600\r\n";
                            response += "Location: /create\r\n\r\n";
                            return response;
                        }
                    }
                }
            }
        }
    }
    
    return generateHTMLResponse("<h1>Error</h1><p>Failed to authenticate with Facebook</p>");
}

// Main request handler
std::string handleRequest(const std::string& request, SOCKET clientSocket) {
    // Check for API requests first
    if (request.find("GET /api/posts ") != std::string::npos) {
        std::string json = "[";
        for (size_t i = 0; i < posts.size(); i++) {
            json += "{";
            json += "\"id\":" + std::to_string(posts[i].id) + ",";
            json += "\"title\":\"" + posts[i].title + "\",";
            json += "\"author\":\"" + posts[i].author + "\",";
            json += "\"content\":\"" + posts[i].content + "\",";
            
            char buffer[80];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&posts[i].timestamp));
            json += "\"timestamp\":\"" + std::string(buffer) + "\"";
            
            json += "}";
            if (i != posts.size() - 1) json += ",";
        }
        json += "]";
        return generateJSONResponse(json);
    }
    // Handle Facebook login
    else if (request.find("GET /login/facebook ") != std::string::npos) {
        return handleFacebookLogin();
    }
    // Handle Facebook auth callback
    else if (request.find("GET /auth/facebook ") != std::string::npos) {
        size_t codePos = request.find("?code=");
        if (codePos != std::string::npos) {
            size_t endPos = request.find(" ", codePos);
            std::string code = request.substr(codePos + 6, endPos - (codePos + 6));
            return handleFacebookAuth(code);
        }
    }
    // Handle home page
    else if (request.find("GET / ") != std::string::npos) {
        return getAllPostsHTML();
    }
    // Handle create post form
    else if (request.find("GET /create ") != std::string::npos) {
        // Check for session to pre-fill author
        std::string author = "";
        size_t cookiePos = request.find("Cookie: ");
        if (cookiePos != std::string::npos) {
            size_t endPos = request.find("\r\n", cookiePos);
            std::string cookie = request.substr(cookiePos + 8, endPos - (cookiePos + 8));
            if (sessions.count(cookie)) {
                UserSession& session = sessions[cookie];
                if (session.platform == "facebook") {
                    // We could fetch the user's name from Facebook here
                    author = "Facebook User";
                }
                else if (session.platform == "twitter") {
                    author = "Twitter User";
                }
            }
        }
        return getCreatePostForm(author);
    }
    // Handle POST to create post
    else if (request.find("POST /create ") != std::string::npos) {
        handleCreatePost(request, clientSocket);
        return ""; // Response already sent in handleCreatePost
    }
    
    // Default response
    return generateHTMLResponse("<h1>Post Server</h1><p>Endpoint not found</p>");
}

int main() {
    WSADATA wsaData;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[10240] = {0}; // Larger buffer for requests
    
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }
    
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    
    // Start listening
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    
    std::cout << "Social Media Post Server listening on port " << PORT << std::endl;
    std::cout << "Access at: " << SERVER_URL << std::endl;
    
    while (true) {
        // Accept new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }
        
        // Read the request
        int bytesRead = recv(new_socket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            std::string request(buffer, bytesRead);
            
            // Handle the request and get response
            std::string response = handleRequest(request, new_socket);
            
            // Send the response if not already sent (like in redirects)
            if (!response.empty()) {
                send(new_socket, response.c_str(), response.length(), 0);
            }
        }
        
        // Clear the buffer
        memset(buffer, 0, sizeof(buffer));
        
        // Close the socket
        closesocket(new_socket);
    }
    
    // Cleanup
    closesocket(server_fd);
    curl_global_cleanup();
    WSACleanup();
    return 0;
}