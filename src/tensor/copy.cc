#include "nntile/tensor/copy.hh"
#include "nntile/tile/copy.hh"

namespace nntile
{

template<typename T>
void copy_intersection_async(const Tensor<T> &src,
        const std::vector<Index> &src_offset, const Tensor<T> &dst,
        const std::vector<Index> &dst_offset)
{
    // Check inputs
    if(src.ndim != src_offset.size())
    {
        throw std::runtime_error("src.ndim != src_offset.size()");
    }
    if(src.ndim != dst.ndim)
    {
        throw std::runtime_error("src.ndim != dst.ndim");
    }
    if(dst.ndim != dst_offset.size())
    {
        throw std::runtime_error("dst.ndim != dst_offset.size()");
    }
    Index ndim = src.ndim;
    // Treat special case of ndim=0
    if(ndim == 0)
    {
        copy_intersection_async(src.get_tile(0), src_offset, dst.get_tile(0),
                dst_offset);
        return;
    }
    // Treat non-zero ndim
    for(Index i = 0; i < src.grid.nelems; ++i)
    {
        auto src_tile_index = src.grid.linear_to_index(i);
        auto src_tile_offset(src_offset);
        for(Index k = 0; k < ndim; ++k)
        {
            src_tile_offset[k] += src_tile_index[k] * src.basetile_shape[k];
        }
        auto src_tile = src.get_tile(i);
        for(Index j = 0; j < dst.grid.nelems; ++j)
        {
            auto dst_tile_index = dst.grid.linear_to_index(j);
            auto dst_tile_offset(dst_offset);
            for(Index k = 0; k < ndim; ++k)
            {
                dst_tile_offset[k] += dst_tile_index[k] *
                    dst.basetile_shape[k];
            }
            copy_intersection_async<T>(src_tile, src_tile_offset,
                    dst.get_tile(j), dst_tile_offset);
        }
    }
}

template
void copy_intersection_async(const Tensor<float> &src,
        const std::vector<Index> &src_offset, const Tensor<float> &dst,
        const std::vector<Index> &dst_offset);

template
void copy_intersection_async(const Tensor<double> &src,
        const std::vector<Index> &src_offset, const Tensor<double> &dst,
        const std::vector<Index> &dst_offset);

} // namespace nntile

