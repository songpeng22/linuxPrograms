#pragma once
#include <string>

using namespace std;

class Crypto
{
public:
    Crypto();
    ~Crypto();

    int     EncryptAES(string plainText, string& cipherText);
    int     DecryptAES(string cipherText, string& plainText);
    int     EncryptAES_GCM(string plainText, string& cipherText);
    int     DecryptAES_GCM(string cipherText, string& plainText);
    short   ReadEncryptedFile(const string& fileName, string& decryptedText);

    static const short E_SUCCESS = 0;
    static const short E_NO_CHANNEL_SUPPORT = 1;
    static const short E_BAD_STATE = 2;
    static const short E_INVALID_PARAMETER = 3;
    static const short E_DATA_INTEGRITY = 4;
    static const short E_HASH_VERIFICATION_FAILED = 5;

private:
    void CreateAccountInfo();
    
    static const char allowedCharacters[64];
    static const int TAG_SIZE;

    string key;                 // private key
    string iv;                  // Initialization vector
    string authenticationStr;   // authentication string

    
};

