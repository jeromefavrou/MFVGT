#ifndef __DEVICE_HPP__
#define __DEVICE_HPP__

#include "../includes/container.hpp"

#include <format>
#include <chrono>
#include <future>
#include <mutex>

/// @brief class ui definit un materiel contenant des conteneur
class Device  : public utilitys::MainPathSharedTemplate
{
    public:
        Device( const std::string & _name);
        Device( const Device & _lref);
        Device( const Device && _rref);
        Device & operator=(const Device & _lref);
        Device & operator=(const Device && _rref);

        std::vector<Container> & get_containers(void);
        const std::string get_name(void)const;
        void update(   std::vector<std::string> & _files,bool _forceUpdate = false , unsigned int _n_thread = 0);
    protected:

        std::vector<Container> m_containers;
        std::string m_name;

        bool m_update;

    private :

        std::mutex m_mutex;

        void findDevice_th( const std::vector<std::string>::iterator & _beg , const std::vector<std::string>::iterator & _end,  std::vector<std::string> & _out );
};

typedef std::shared_ptr<std::vector<Device>> ptrVecDevice;


/// @brief initialise avec un nom
/// @param _name 
Device::Device( const std::string & _name):m_name(_name),m_update(false)
{
}

/// @brief operateur de copy
/// @param _lref 
Device & Device::operator=(const Device & _lref)
{
    this->m_containers = _lref.m_containers;
    this->m_name =  _lref.m_name;
    this->m_update = _lref.m_update;

    return *this;
}

/// @brief operateur de copy
/// @param _lref 
Device & Device::operator=(const Device && _rref)
{
    this->m_containers = _rref.m_containers;
    this->m_name =  _rref.m_name;
    this->m_update = _rref.m_update;

    return *this;
}

/// @brief constructeur de copy
/// @param _lref 
Device::Device( const Device & _lref)
{
    *this = _lref;
}

/// @brief constructeur de copy
/// @param _lref 
Device::Device( const Device && _rref)
{
    *this = _rref;
}


/// @brief retourn le conteneur
/// @return std::vector<Container>
std::vector<Container> & Device::get_containers(void)
{
    return this->m_containers;
}

/// @brief retourn le nom du materiel
/// @return str
const std::string Device::get_name(void)const
{
    return this->m_name;
}

/// @brief 
/// @param _beg debut de list d'entré
/// @param _end fin de liste d'entré
/// @param _out list de sous chemin chemin
void Device::findDevice_th( const std::vector<std::string>::iterator & _beg , const std::vector<std::string>::iterator & _end,  std::vector<std::string> & _out )
{
    for(auto f = _beg ; f != _end ; f++)
    {
        auto tmp = utilitys::sep_sub_and_name( *f );
        
        bool found =false;
        //! ici on cherche u materiel
        
        while ( std::get<0>(tmp).size() > 0)
        {
            tmp = utilitys::sep_sub_and_name( std::get<0>(tmp) );

            std::string tmpName = std::get<1>(tmp);
            utilitys::upper(tmpName);
            
            if( tmpName == this->m_name )
            {
                found = true;
                break;
            }
        }
        
        if( found )
        {
                this->m_mutex.lock();

                _out.push_back( f->substr( this->atMainPath()->size() , f->size()) );
                
                //supprime la racine si presente
                if(_out.back().front() == '/' || _out.back().front() == '\\')
                    _out.back().erase(_out.back().begin());

                this->m_mutex.unlock();
        }
           
    }
}

/// @brief met a jour les conteneur en scanant une racine pour y trouver le materiel et les conteneur
/// @param _mainPath racine a scanné
/// @param _files  tout les chemin fichier compatible
/// @param _forceUpdate force la mise a jour si deja scanné
void Device::update(  std::vector<std::string> & _files , bool _forceUpdate ,unsigned int _n_thread)
{

    if( this->m_update && !_forceUpdate)
        return ;
        
        
    std::vector<std::string>  pathDevice ;

    if(_n_thread < 2 || _files.size() < 2)
        this->findDevice_th( _files.begin() , _files.end() , pathDevice);
    else
    {
        std::vector< std::thread > tmp_th;

        int divider = _files.size() / _n_thread;

        if( divider == 0)
        {
            this->findDevice_th( _files.begin() , _files.end() , pathDevice);
        }
        else
        {
            for( unsigned int i = 0 ; i < _n_thread ; i++)
            {
                if( i == _n_thread-1)
                {
                    tmp_th.emplace_back( &Device::findDevice_th, this, _files.begin() + divider * i , _files.end() , std::ref(pathDevice) );
                }
                else
                {
                    tmp_th.emplace_back( &Device::findDevice_th, this, _files.begin() + divider * i , _files.begin() + divider * (i+1)  ,std::ref(pathDevice) );
                }
            }

            for( auto & th : tmp_th)
                th.join();
            }
    }



    //recherche les differant contenaire
    
    for(auto & c : m_containers)
    {
        c.clear();

        for(const auto &  pd : pathDevice)
        {
            std::string tmpStr = pd;

            utilitys::upper(tmpStr);
            auto found = tmpStr.find( c.get_name() );

            if( found!=std::string::npos)
            {
                
                auto pathName = utilitys::sep_sub_and_name(pd);
                auto nameExt = utilitys::sep_name_and_ext(std::get<1>(pathName));
                
                //ajoute si l'extension est supporté
                if( c.get_authExt().find( std::get<1>(nameExt) ) != std::string::npos )
                {
                    Version tmp ;

                    tmp.device = this->m_name;
                    tmp.dir = std::get<0>(pathName);
                    
                    tmp.name = std::get<0>(nameExt);
                    tmp.id = utilitys::regSearch(c.get_reg_id() , tmp.name);
                    
                    if(tmp.id.size() == 0)
                        tmp.id = tmp.name;

                    tmp.autor =  std::move(utilitys::regSearch(c.get_reg_autor() , tmp.name));
                    tmp.part =  std::move(utilitys::regSearch(c.get_reg_part() , tmp.name));
                    tmp.version =  std::move(utilitys::regSearch(c.get_reg_version() , tmp.name));

                    
                    tmp.extension = std::get<1>(nameExt);

                    std::string tmpFileName =  *this->atMainPath() + "\\"+ tmp.dir +"\\"+ tmp.name + "."+ tmp.extension;


                    // Obtenez le temps de dernière modification du fichier
                    try
                    {
                        std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(tmpFileName);
                        tmp.modifDate = std::string(std::format("{}",lastWriteTime)).substr(0,10) ;
                    }
                    catch(const std::exception & e)
                    {
                        tmp.add_error(Version::EF_ACCES);
                        std::cerr << e.what() << std::endl;
                    }

                    try
                    {
                        tmp.createDate = utilitys::get_createDate(tmpFileName);
                    }
                    catch(const std::exception & e)
                    {
                        tmp.add_error(Version::EF_ACCES);
                        std::cerr << e.what() << std::endl;
                    }

                    c.add2GrpVersion( std::move(tmp) );
                }
                
            }
        } 

    }
    pathDevice.clear();
    pathDevice.shrink_to_fit();
    
    this->m_update  =true;
}

static std::tuple<std::string,ptrVecDevice> load(const std::string & _ressourcePath)
{
    std::clog << "load configuration ...   " << std::endl;
    const auto start{std::chrono::steady_clock::now()};

    ptrVecDevice devices;


    //charge les ficheir de maniere asynchrone // utlise 3 thread
    std::future< std::vector<std::string> > deviceTh = std::async ( utilitys::readList, _ressourcePath + "\\devicesList.csv" , true);
    std::future< std::vector<std::string> > contTh = std::async ( utilitys::readList, _ressourcePath + "\\containerList.csv" , true);
    std::future< std::vector<std::string> > parameterTh = std::async (utilitys::readList, _ressourcePath + "\\parameters.csv" , false);
    
    std::vector<std::string>&& lsContainer = contTh.get();
    std::vector<std::string>&& lsParameter = parameterTh.get();
    std::vector<std::string>&& lsDevices = deviceTh.get();


    if(  lsContainer.size() == 0)
        throw std::logic_error("no containers found");
    
    if( lsDevices.size() == 0 )
        throw std::logic_error("no devices subpath found");
    
    if(lsParameter.size() == 0)
        throw std::logic_error("no parametres found");


    std::string && mainPath =  utilitys::getParamValue("MAIN_PATH" , lsParameter , true);


    if(mainPath.size() == 0)
        throw std::logic_error("no mainPath  found");

    if(mainPath.back() == '\\' || mainPath.back() == '/')
        mainPath.pop_back();


    devices = ptrVecDevice(new std::vector<Device>());

    //contruis une liste de contenaire patron pour ne pas repeter getParamValue
    std::vector<Container> tmpCont;
    for(const auto & cnt : lsContainer)
    {
        tmpCont.emplace_back( cnt );
        tmpCont.back().set_authExt( std::move(utilitys::getParamValue("AUTHEXT[" + cnt +"]" , lsParameter , true)) );

        tmpCont.back().set_reg_version( std::move(utilitys::getParamValue("VERSIONREGEX[" + cnt +"]" , lsParameter , true)));
        tmpCont.back().set_reg_id( std::move(utilitys::getParamValue("IDREGEX[" + cnt +"]" , lsParameter , true)));
        tmpCont.back().set_reg_autor( std::move(utilitys::getParamValue("AUTORREGEX[" + cnt +"]" , lsParameter , true)));
        tmpCont.back().set_reg_part( std::move(utilitys::getParamValue("PARTREGEX[" + cnt +"]" , lsParameter , true)));
    }

    //construit la liste de materiel
    for(const auto & name : lsDevices)
    {
        //cherche si pas deja inlcu

        bool found = false;
        for( const auto & dev : *devices)
            if(name == dev.get_name())
                found = true;

        //si le cas passe au suivant
        if( found)
            continue;

        //ajoute un materiel
        devices->emplace_back( name );

        //ajoute les contenaire au materiel
        for(const auto & cnt : tmpCont)
        {
            devices->back().get_containers().emplace_back( std::move(cnt.get_name()) );
            devices->back().get_containers().back().set_authExt( std::move(cnt.get_authExt()));

            devices->back().get_containers().back().set_reg_version(  std::move(cnt.get_reg_version()) );
            devices->back().get_containers().back().set_reg_id(  std::move(cnt.get_reg_id()) );
            devices->back().get_containers().back().set_reg_autor(  std::move(cnt.get_reg_autor()) );
            devices->back().get_containers().back().set_reg_part(  std::move(cnt.get_reg_part()) );
        }
    }

    const auto end{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{end - start};

    std::clog <<"succes load in : " << elapsed_seconds.count() * 1000 << " ms" <<std::endl;

    return {mainPath , devices};
}

#endif