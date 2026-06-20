#pragma once
#include <cstddef>
#include <memory>
#include <vector>

template<typename T>
class MemoryManager {
    public:
        union Chunk {
            Chunk* next;
            alignas(T) unsigned char storage[sizeof(T)];
        };
    private:
        struct Pool {
            std::unique_ptr<Chunk[]> memory;
            Chunk* bump;
            Chunk* end;

            explicit Pool(size_t n) : memory(new Chunk[n]), bump(memory.get()), end(memory.get() + n) {};

            Chunk* bump_allocate() {
                Chunk* chunk = nullptr;
                if (bump != end) {
                    chunk = bump++;
                }
                return chunk;
            }
        };
        
        std::vector<std::unique_ptr<Pool>> pools;
        Chunk* free_head = nullptr;
        size_t pool_size;

        void addPool() {
            pools.push_back(std::make_unique<Pool>(pool_size));
        }
    public:
        explicit MemoryManager(size_t size) {
            pool_size = size;
            addPool();
        }

        T* allocate() {
            Chunk* chunk = nullptr;
            if (free_head != nullptr) {
                chunk = free_head;
                free_head = free_head->next;
            }
            else {
                Pool* pool = pools.back().get();
                chunk = pool->bump_allocate();
                if (chunk == nullptr) {
                    addPool();
                    pool = pools.back().get();
                    chunk = pool->bump_allocate();
                }
            }
            return new (chunk->storage) T();
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

