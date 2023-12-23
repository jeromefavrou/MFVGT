#ifndef __UTILITYS_HPP__
#define __UTILITYS_HPP__


#include <windows.h>
#include <tchar.h>
#include <vector>
#include <fstream>
#include <string>
#include <locale>
#include <tuple>
#include <filesystem>
#include <iostream>
#include <regex>

#include <cryptopp/cryptlib.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>

#include <gtkmm-3.0/gtkmm/container.h>

/// ensemble de fonction static utilitaire
namespace utilitys
{
    /// @brief copy d'une chaine str dans le presse papier via l'api windows
    /// @param _cpy chaine de caratere string
    static void cpyToPP(const std::string _cpy)
    {
        // Ouvrir le presse-papiers
        if (OpenClipboard(NULL)) {
            // Vider le presse-papiers
            EmptyClipboard();

            // Allouer de la mémoire pour la chaîne de caractères
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strlen(_cpy.c_str()) + 1);
            if (hMem != NULL) {
                // Obtenir un pointeur vers la mémoire allouée
                char* pMem = static_cast<char*>(GlobalLock(hMem));
                if (pMem != NULL) {
                    // Copier la chaîne de caractères dans la mémoire allouée
                    strcpy(pMem, _cpy.c_str());

                    // Libérer la mémoire
                    GlobalUnlock(hMem);

                    // Mettre la mémoire dans le presse-papiers
                    SetClipboardData(CF_TEXT, hMem);

                    // Fermer le presse-papiers
                    CloseClipboard();

                } 
            } 
        } 
    }

    /// @brief retourn la métadonné date de creation d'un fichier !!! le rettour n'est pas garentie
    /// @param _file fichier source
    /// @return  retour la date si ok sinon retourn une chaine vide
    static std::string get_createDate(std::string const & _file)
    {
        // Obtenir le handle du fichier
        HANDLE hFile = CreateFile(_file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == INVALID_HANDLE_VALUE) 
            return "";
        

        // Obtenir les informations sur le fichier, y compris la date de création
        FILETIME creationTime, lastAccessTime, lastWriteTime;
        if (GetFileTime(hFile, &creationTime, &lastAccessTime, &lastWriteTime)) 
        {
            // Convertir FILETIME en système local time
            SYSTEMTIME sysCreationTime;
            if (FileTimeToSystemTime(&creationTime, &sysCreationTime)) 
            {
                // Afficher la date de création

                std::stringstream ss;
                ss << (sysCreationTime.wYear < 10 ? "0" : "")<< sysCreationTime.wYear << "-" <<  (sysCreationTime.wMonth < 10 ? "0" : "") << sysCreationTime.wMonth << "-" << (sysCreationTime.wDay < 10 ? "0" : "") << sysCreationTime.wDay << std::endl;
                
                std::string tmp;
                std::getline(ss , tmp);

                CloseHandle(hFile);
                return tmp;
            } 
        }

        // Fermer le handle du fichier
        CloseHandle(hFile);

        return "";
    }


    /// @brief supprime les enfant d'un conteneur Gtkmm
    /// @param _wref //Conteneur parent Gtkmm
    static void gtkmmRemoveChilds(Gtk::Container & _wref)
    {
        for (auto& child : _wref.get_children()) 
            _wref.remove(*child);
    }

    /// @brief augmente la casse des lettre d'une chaine
    /// @param _refstr chaine a traité
    static void upper(std::string & _refstr)
    {
        for(std::string::iterator it = _refstr.begin() ; it != _refstr.end() ; it++ )
            if(*it >= 0x61 && *it <= 0x7A)
                *it = *it - char(32) ;
    }

    /// @brief augmentre la casse d'une liste de chaine str 
    /// @param _vecstrref liste de chaine a traiter
    static void upperList(std::vector<std::string > & _vecstrref)
    {
        for(auto it = _vecstrref.begin() ; it != _vecstrref.end() ; it++ )
            upper(*it);
    }
    //separe un path en sous path et nom

    /// @brief sépare une chemin de fichier en chemin et nom de fichier
    /// @param _str chemin a séparer
    /// @return tuple de chaine str contenant le chemin <0> et le nom de fichier <1>
    static std::tuple<std::string , std::string> sep_sub_and_name(std::string const & _str)
    {
        int idx = 0;

        for(auto i = _str.size() ; i >= 0 ; i--)
        {
            if(_str[i] == '\\' || _str[i] == '/' )
            {
                idx = i ; 
                break;
            }
        }

        if( idx <= 0)
            return {"" , _str};

        return {_str.substr(0 , idx) , _str.substr(idx+1 , _str.size())};
    }

    /// @brief sépare une nom de fichier et son extention
    /// @param _str 
    /// @return tuple de str contenant les nom fichier <0> et son extension<1>
    static std::tuple<std::string , std::string> sep_name_and_ext(std::string const & _str)
    {
        int idx = 0;

        for(auto i = _str.size() ; i >= 0 ; i--)
        {
            if(_str[i] == '.' )
            {
                idx = i ; 
                break;
            }
        }

        if( idx <= 0)
            return {_str , ""};

        return {_str.substr(0 , idx) , _str.substr(idx+1 , _str.size())};
    }

    /// @brief sépare une chaine str en 2 a l'aide du premier caratere cible trouvé
    /// @tparam _c caractere cible pour la séparation
    /// @param _str chaine a séparé
    /// @return retourn un tuple de str des 2 morceau de chaine séparé
    template<char _c> std::tuple<std::string , std::string> sep_string(std::string const & _str , bool _reverse = false)
    {
        int idx = 0;

        if(!_reverse)
        {
           for(size_t i = 0 ; i<_str.size() ; i++)
            {
                if(_str[i] == _c )
                {
                    idx = i ; 
                    break;
                }
            } 

            if( idx <= 0)
                return {_str , ""};

        }
        else
        {
            for(size_t i = _str.size()-1 ; i>=0 ; i--)
            {
                if(_str[i] == _c )
                {
                    idx = i ; 
                    break;
                }
            } 

            if( idx <= 0)
                return {"" , _str};
                
            
        }
        
        return {_str.substr(0 , idx) , _str.substr(idx+1 , _str.size())};
        
    }


    /// @brief liste tout les fichier d'un répertoir mais de maniere recursive
    /// @param chemin //Chemin vers du repertoir cible 
    /// @param fichiers //list a traité
    static void listerFichiers(const std::filesystem::path& chemin, std::vector<std::string>& fichiers)
    {
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(chemin))
            {
                if (std::filesystem::is_directory(entry.status()))
                {
                    // Appel récursif pour traiter les sous-répertoires
                    listerFichiers(entry.path(), fichiers);
                } else if (std::filesystem::is_regular_file(entry.status()))
                {
                    // Ajouter le fichier au vecteur
                    fichiers.push_back(chemin.string() + "\\"+ entry.path().filename().string());
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Erreur : " << e.what() << std::endl;
        }
    }

    /// @brief lit l'enseble des ligne d'un fichier et stock dans un vecteur
    /// @param _filePath chemin du fichier a lire 
    /// @param _upper option d'augmentation de la casse ( integré pour pouvoir etre parrallelisé en meme temps)
    /// @return retourn la liste des ligne lu
    static std::vector<std::string>  readList(const std::string & _filePath , bool _upper = false)
    {
        std::ifstream f;
        //actives les exception sur badbit
        f.exceptions( std::ifstream::badbit);
        
        std::vector<std::string> res;
        res.reserve(100);
        try
        {
            f.open( _filePath );

            if( !f.good())
            {
                f.close();
                throw std::runtime_error("Erreur lors de l'ouverture du fichier : " + _filePath);
            }
                

            std::string tmp;
            
            while (std::getline( f , tmp)) 
                res.push_back( std::move(tmp) );
        }
        catch (const std::ifstream::failure& e) 
        {
            std::cerr  << e.what() << std::endl;
        }

        f.close();

        res.shrink_to_fit();

        if( _upper)
                upperList(res);

        return  res;
    }

    /// @brief cherche la valeur apartenant a une clé dans un fichier 
    /// @param _paramName nom du parametre a trouvé
    /// @param _params liste des ligne ou chercher les parametre
    /// @param _remove_found option d'optimisation , suppression des ligne trouvé au fur et a mesur
    /// @return retourn la valeur du parametre recherché
    static std::string getParamValue( const std::string & _paramName  , std::vector<std::string> & _params , bool _remove_found =false )
    {
        std::string res;

        for( auto  it = _params.begin() ; it != _params.end() ; it++)
        {
        if( it->find("=") != std::string::npos )
        {
                auto sep = utilitys::sep_string<'='>( *it );

                std::string tmpstr = std::get<0>(sep);
                utilitys::upper(tmpstr);

                if( tmpstr == _paramName )
                {
                    res =std::get<1>(sep);

                    if(_remove_found)
                        _params.erase(it);

                    break; 
                }
        }
        }

        return res;
    }

    /// @brief recherche la match principal d'un regex 
    /// @param _reg  regex a utilisé
    /// @param _str chaine a traité avec le regex
    /// @return retourn le regex principal
    static std::string regSearch(const std::string & _reg , const std::string & _str)
    {
        if( _reg.size() == 0 || _str.size() == 0)
            return "";
        
        std::regex pattern(_reg ,  std::regex_constants::ECMAScript | std::regex_constants::icase);

        std::smatch match;
        if (std::regex_search(_str, match, pattern)) 
            return match.str();
        

        return "";
    }

    /// @brief compare 2 fichier grace a leur hash , pas de notion de securité , utilisation de MD5 pour gain de cycle
    /// @param file1 1er fichier
    /// @param file2 2nd fichier
    /// @return retourn vrai ou faux si fichien identique ou nom
    static bool cmpFilesHash(const std::string& file1, const std::string& file2) 
    {
        std::string checksum1 , checksum2;
        CryptoPP::Weak::MD5 hash,hash2 ;

        std::ifstream f(file1 , std::ios::binary);
        if(f)
            CryptoPP::FileSource(f, true, new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(checksum1)));
    
        std::ifstream f2(file2 , std::ios::binary);
        if(f2)
            CryptoPP::FileSource(f2, true, new CryptoPP::HashFilter(hash2, new CryptoPP::StringSink(checksum2)));

        
        f.close();
        f2.close();
        // Compare the checksums
        return checksum1 == checksum2;
    }

    /// @brief compare si 2 fichier ont la mem taille
    /// @param file1 chemin vers 1er fichier
    /// @param file2 chemin vers second fichier
    /// @return retour vrai ou faux si fichier ideteique en taille
    static bool cmpFileSize(const std::string& file1, const std::string& file2)
    {
        std::ifstream stream1(file1, std::ios::binary | std::ios::ate);
        std::ifstream stream2(file2, std::ios::binary | std::ios::ate);

        if (!stream1 || !stream2)
            return false;
        
        unsigned long int s1 = stream1.tellg();
        unsigned long int s2 = stream2.tellg();

        stream1.close();
        stream2.close();

        return s1 == s2;
    }


    static void openWebPage(const std::string& url) 
    {
        ShellExecute(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
};




#endif