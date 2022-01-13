// header for cryptopp
#include <files.h>

#include <cryptlib.h>
using CryptoPP::Exception;

#include <hex.h>
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;

#include <des.h>
using CryptoPP::DES_EDE2;

#include <modes.h>
using CryptoPP::CBC_Mode;

#include <secblock.h>
using CryptoPP::SecByteBlock;

#include <gcm.h>
using CryptoPP::GCM;
using CryptoPP::GCM_TablesOption;

#include <aes.h>
using CryptoPP::AES;

#include <filters.h>
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::StreamTransformationFilter;

#include "larsErr.h"
#include "adctrace.h"
#include "crypto.h"


const char Crypto::allowedCharacters[] = "0123456789abcderfghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const int Crypto::TAG_SIZE = 16;


Crypto::Crypto()
{
    CreateAccountInfo();
}


Crypto::~Crypto()
{
}


void Crypto::CreateAccountInfo()
{
    // create the account information for the AES algorithm
    // AES encryption uses a secret key of a variable length (128-bit, 196-bit or 256-
    // bit). This key is secretly exchanged between two parties before communication
    // begins. DEFAULT_KEYLENGTH= 16 bytes
    key += allowedCharacters[4];
    key += allowedCharacters[55];
    key += allowedCharacters[21];
    key += allowedCharacters[0];
    key += allowedCharacters[61];
    key += allowedCharacters[26];
    key += allowedCharacters[46];
    key += allowedCharacters[59];
    key += allowedCharacters[15];
    key += allowedCharacters[35];
    key += allowedCharacters[39];
    key += allowedCharacters[33];
    key += allowedCharacters[10];
    key += allowedCharacters[3];
    key += allowedCharacters[8];
    key += allowedCharacters[19];
    
    iv += allowedCharacters[51];
    iv += allowedCharacters[4];
    iv += allowedCharacters[48];
    iv += allowedCharacters[2];
    iv += allowedCharacters[25];
    iv += allowedCharacters[60];
    iv += allowedCharacters[29];
    iv += allowedCharacters[36];
    iv += allowedCharacters[8];
    iv += allowedCharacters[23];
    iv += allowedCharacters[13];
    iv += allowedCharacters[26];
    iv += allowedCharacters[2];
    iv += allowedCharacters[57];
    iv += allowedCharacters[10];
    iv += allowedCharacters[7];

    authenticationStr += allowedCharacters[6];
    authenticationStr += allowedCharacters[57];
    authenticationStr += allowedCharacters[6];
    authenticationStr += allowedCharacters[53];
    authenticationStr += allowedCharacters[26];
    authenticationStr += allowedCharacters[33];
    authenticationStr += allowedCharacters[37];
    authenticationStr += allowedCharacters[12];
    authenticationStr += allowedCharacters[48];
    authenticationStr += allowedCharacters[51];
    authenticationStr += allowedCharacters[26];
    authenticationStr += allowedCharacters[35];
    authenticationStr += allowedCharacters[17];
    authenticationStr += allowedCharacters[5];
    authenticationStr += allowedCharacters[8];
    authenticationStr += allowedCharacters[28];
}


int Crypto::EncryptAES(string plainText, string& cipherText)
{
    int ret = E_SUCCESS;

    // confidentiality only 
    CryptoPP::AES::Encryption aesEncryption((byte *)key.c_str(), CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, (byte *)iv.c_str());
    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipherText));

    stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plainText.c_str()), plainText.length() + 1);
    stfEncryptor.MessageEnd();

    return ret;
}


int Crypto::DecryptAES(string cipherText, string& plainText)
{
    int ret = E_SUCCESS;

    CryptoPP::AES::Decryption aesDecryption((byte *)key.c_str(), CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, (byte *)iv.c_str());
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(plainText));

    stfDecryptor.Put(reinterpret_cast<const unsigned char*>(cipherText.c_str()), cipherText.size());
    stfDecryptor.MessageEnd();

    return ret;
}


int Crypto::EncryptAES_GCM(string plainText, string &cipherText)
{
    int ret = E_SUCCESS;

    // AES with GCM (confidentiality and authentication)
    try
    {
        GCM< AES >::Encryption aesEncryption;
        aesEncryption.SetKeyWithIV((byte *)key.c_str(), CryptoPP::AES::DEFAULT_KEYLENGTH, (byte *)iv.c_str(), CryptoPP::AES::DEFAULT_KEYLENGTH);


        CryptoPP::AuthenticatedEncryptionFilter encryptor(aesEncryption,
            new StringSink(cipherText), false, TAG_SIZE
            ); // AuthenticatedEncryptionFilter

        // AuthenticatedEncryptionFilter::ChannelPut
        //  defines two channels: "" (empty) and "AAD"
        //   channel "" is encrypted and authenticated
        //   channel "AAD" is authenticated
        encryptor.ChannelPut("AAD", (const byte*)authenticationStr.data(), authenticationStr.size());
        encryptor.ChannelMessageEnd("AAD");

        // Authenticated data *must* be pushed before
        //  Confidential/Authenticated data. Otherwise
        //  we must catch the BadState exception
        encryptor.ChannelPut("", (const byte*)plainText.data(), plainText.size());
        encryptor.ChannelMessageEnd("");

    }
    catch (CryptoPP::BufferedTransformation::NoChannelSupport)
    {
        // The tag must go in to the default channel:
        //  "unknown: this object doesn't support multiple channels"
        ret = E_NO_CHANNEL_SUPPORT;
    }
    catch (CryptoPP::AuthenticatedSymmetricCipher::BadState)
    {
        // Pushing PDATA before ADATA results in:
        //  "GMC/AES: Update was called before State_IVSet"
        ret = E_NO_CHANNEL_SUPPORT;
    }
    catch (CryptoPP::InvalidArgument)
    {
        ret = E_INVALID_PARAMETER;
    }

    return ret;
}


int Crypto::DecryptAES_GCM(string cipherText, string& plainText)
{
    int ret = E_SUCCESS;

    try
    {
        // AES with CCM (confidentiality and authentication)
        GCM< AES >::Decryption aesDecryption;
        aesDecryption.SetKeyWithIV((byte *)key.c_str(), CryptoPP::AES::DEFAULT_KEYLENGTH, (byte *)iv.c_str(), CryptoPP::AES::DEFAULT_KEYLENGTH);

        // Break the cipher text out into it's
        //  components: Encrypted Data and MAC Value
        string encryptedData = cipherText.substr(0, cipherText.length() - TAG_SIZE);
        string mac = cipherText.substr(cipherText.length() - TAG_SIZE);

        // Object will not throw an exception during decryption\verification if verification fails.

        CryptoPP::AuthenticatedDecryptionFilter decryptor(aesDecryption, NULL,
                                                          CryptoPP::AuthenticatedDecryptionFilter::MAC_AT_BEGIN |
                                                          CryptoPP::AuthenticatedDecryptionFilter::THROW_EXCEPTION, TAG_SIZE);

        // The order of the following calls are important
        decryptor.ChannelPut("", (const byte*)mac.data(), mac.size());
        // adata not recovered - sent via clear channel
        decryptor.ChannelPut("AAD", (const byte*)authenticationStr.data(), authenticationStr.size());
        decryptor.ChannelPut("", (const byte*)encryptedData.data(), encryptedData.size());

        // If the object throws, it will most likely occur
        //  during ChannelMessageEnd()
        decryptor.ChannelMessageEnd("AAD");
        decryptor.ChannelMessageEnd("");

        // If the object does not throw, here's the only
        //  opportunity to check the data's integrity
        bool dataIntegrityOK = false;
        dataIntegrityOK = decryptor.GetLastResult();
        if (dataIntegrityOK == false)
            ret = E_DATA_INTEGRITY;

        // remove Plain text recovered from enc.data()
        decryptor.SetRetrievalChannel("");
        size_t bytesRetrieved = (size_t)decryptor.MaxRetrievable();
        if (bytesRetrieved > 0)
        {
            plainText.resize(bytesRetrieved);
            decryptor.Get((byte*)plainText.data(), bytesRetrieved);
        }
    }
    catch (CryptoPP::InvalidArgument&)
    {
        ret = E_INVALID_PARAMETER;
    }
    catch (CryptoPP::AuthenticatedSymmetricCipher::BadState&)
    {
        // Pushing PDATA before ADATA results in:
        //  "GMC/AES: Update was called before State_IVSet"
        ret = E_BAD_STATE;
    }
    catch (CryptoPP::HashVerificationFilter::HashVerificationFailed&)
    {
        ret = E_HASH_VERIFICATION_FAILED;
    }
	catch (std::exception&)
	{
		ret = E_INVALID_PARAMETER;
	}

    return ret;
}


short Crypto::ReadEncryptedFile(const string& fileName, string& decryptedText)
{
    string cipherText;

    try
    {
        // read encoded file
        CryptoPP::FileSource(fileName.c_str(), true, new CryptoPP::HexDecoder(new CryptoPP::StringSink(cipherText)));

        // Decrypt
        if (DecryptAES_GCM(cipherText, decryptedText) != Crypto::E_SUCCESS)
        {
            g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tdecryption error, file %s corrupted", __FUNCTION__, fileName.c_str());
            return LarsErr::E_FILE_CORRUPT;
        }
    }
    catch (CryptoPP::Exception &e)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\t%s", __FUNCTION__, e.GetWhat().c_str());
        return LarsErr::E_FILE_NOT_FOUND;
    }

    return LarsErr::E_SUCCESS;
}
