# FastMap (associative container)  

    template<typename Key, typename Value, typename MaxSize = uint32_t, typename MaxCapacity = uint32_t>
    class FastMap
    {
     ...
    }  
      
    // More pre parametrized classes (Utility)
    template<typename Key, typename Value, typename MaxCapacity = uint32_t>  
    - FastMapMaxSize8Bit  
    - FastMapMaxSize16Bit  
    - FastMapMaxSize32Bit  
    - FastMapMaxSize64Bit  
        
*Example usage*

    FastMap<int, int, uint16_t> map(1024); // uint16_t : I don't need more than 65535 elements (optimization)  
                                           // 1024 : capacity must be power of 2  
    map.Insert(5, 20);  
    map.Erase(5);  
    map.Find(5);  

**Important notes**
 * Not final product, Find, Insert, Erase will be improved as soon as possible
 * Keys larger than 64 bit are NOT SUPPORTED yet
 * Memory buffer cannot grow when Inserting, so construct FastMap with desired capacity needed
 * When constructing with capacity, it must be power of 2

**Properties**
 * Finding keys that has a low chance to occur in the Map (SUPER FAST)
 * Finding keys that has a high chance to occur in the Map (FAST)
 * Erase not implemented yet
 * Insert really slow, will be improved soon

**Feel free to contact**,  
 hsdxpro@gmail.com
