#include <cstddef>

template<typename T>
class ChunkPool {
    private:
        union Chunk {
            Chunk* next;
            alignas(T) unsigned char storage[sizeof(T)];
        };

        Chunk* pool_memory;
        Chunk* free_head;

    public:
        ChunkPool(size_t size) {
            pool_memory = new Chunk[size];
            
            for (size_t i = 0; i < size - 1; ++i) {
                pool_memory[i].next = &pool_memory[i + 1];
            }
            
            pool_memory[size - 1].next = nullptr;

            free_head = &pool_memory[0];
        }

        ~ChunkPool() {
            delete[] pool_memory;
        }

        T* allocate() {
            if (free_head == nullptr) {
                return nullptr;
            }

            Chunk* chunk = free_head;
            free_head = free_head->next;

            T* object = new (chunk->storage) T();

            return object;
        }

        void deallocate(T* object) {
            if (object == nullptr) {
                return;
            }

            object->~T();

            Chunk* chunk = reinterpret_cast<Chunk*>(object);
            
            chunk->next = free_head;
            free_head = chunk;
        }
};