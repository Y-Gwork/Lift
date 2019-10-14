

#include <cuda.h>
namespace lift {
template<typename T = char>
class CudaBuffer {
 public:
    CudaBuffer(size_t count = 0) { alloc(count); }
    ~CudaBuffer() { free(); }
    void alloc(size_t count) {
        free();
        alloc_count_ = count_ = count;
        if (count_) {
            CUDA_CHECK(cudaMalloc(&d_ptr_, alloc_count_ * sizeof(T)));
        }
    }
    void allocIfRequired(size_t count) {
        if (count <= count_) {
            count_ = count;
            return;
        }
        alloc(count);
    }
    CUdeviceptr get() const { return reinterpret_cast<CUdeviceptr>( d_ptr_ ); }
    CUdeviceptr get(size_t index) const { return reinterpret_cast<CUdeviceptr>( d_ptr_ + index ); }
    void free() {
        count_ = 0;
        alloc_count_ = 0;
        CUDA_CHECK(cudaFree(d_ptr_));
        d_ptr_ = nullptr;
    }
    CUdeviceptr release() {
        CUdeviceptr current = reinterpret_cast<CUdeviceptr>( d_ptr_ );
        count_ = 0;
        alloc_count_ = 0;
        d_ptr_ = nullptr;
        return current;
    }
    void upload(const T* data) {
        CUDA_CHECK(cudaMemcpy(d_ptr_, data, count_ * sizeof(T), cudaMemcpyHostToDevice));
    }

    void download(T* data) const {
        CUDA_CHECK(cudaMemcpy(data, d_ptr_, count_ * sizeof(T), cudaMemcpyDeviceToHost));
    }
    void downloadSub(size_t count, size_t offset, T* data) const {
        assert(count + offset < alloc_count_);
        CUDA_CHECK(cudaMemcpy(data, d_ptr_ + offset, count * sizeof(T), cudaMemcpyDeviceToHost));
    }
    size_t count() const { return count_; }
    size_t reservedCount() const { return alloc_count_; }
    size_t byteSize() const { return alloc_count_ * sizeof(T); }

 private:
    size_t count_ = 0;
    size_t alloc_count_ = 0;
    T* d_ptr_ = nullptr;
};
}  // namespace