
#include <algorithm>
#include <cfloat>
#include <iostream>
#include <iomanip>
#include <map>
#include <limits>

#include <AMReX_BLassert.H>
#include <AMReX_MultiFab.H>
#include <AMReX_ParallelDescriptor.H>
#include <AMReX_BLProfiler.H>
#include <AMReX_iMultiFab.H>
#include <AMReX_FabArrayUtility.H>

#ifdef AMREX_MEM_PROFILING
#include <AMReX_MemProfiler.H>
#endif

namespace amrex {

namespace
{
    bool initialized = false;
#ifdef AMREX_MEM_PROFILING
    int num_multifabs     = 0;
    int num_multifabs_hwm = 0;
#endif
}

Real
MultiFab::Dot (const MultiFab& x, int xcomp,
	       const MultiFab& y, int ycomp,
	       int numcomp, int nghost, bool local)
{
    BL_ASSERT(x.boxArray() == y.boxArray());
    BL_ASSERT(x.DistributionMap() == y.DistributionMap());
    BL_ASSERT(x.nGrow() >= nghost && y.nGrow() >= nghost);

    BL_PROFILE("MultiFab::Dot()");

    Real sm = amrex::ReduceSum(x, y, nghost,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& xfab, FArrayBox const& yfab) -> Real
    {
        return xfab.dot(bx,xcomp,yfab,bx,ycomp,numcomp);
    });

    if (!local) ParallelAllReduce::Sum(sm, ParallelContext::CommunicatorSub());

    return sm;
}

Real
MultiFab::Dot (const MultiFab& x, int xcomp,
	       int numcomp, int nghost, bool local)
{
    BL_ASSERT(x.nGrow() >= nghost); 

    Real sm = amrex::ReduceSum(x, nghost,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& xfab) -> Real
    {
        return xfab.dot(bx,xcomp,numcomp);
    });

    if (!local) ParallelAllReduce::Sum(sm, ParallelContext::CommunicatorSub());

    return sm;
}


Real
MultiFab::Dot (const iMultiFab& mask,
               const MultiFab& x, int xcomp,
	       const MultiFab& y, int ycomp,
	       int numcomp, int nghost, bool local)
{
    BL_ASSERT(x.boxArray() == y.boxArray());
    BL_ASSERT(x.boxArray() == mask.boxArray());
    BL_ASSERT(x.DistributionMap() == y.DistributionMap());
    BL_ASSERT(x.DistributionMap() == mask.DistributionMap());
    BL_ASSERT(x.nGrow() >= nghost && y.nGrow() >= nghost);
    BL_ASSERT(mask.nGrow() >= nghost);

    Real sm = amrex::ReduceSum(x, y, mask, nghost,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& xfab, FArrayBox const& yfab,
                               IArrayBox const& mskfab) -> Real
    {
        return xfab.dotmask(mskfab, bx, xcomp, yfab, bx, ycomp, numcomp);
    });

    if (!local)
        ParallelAllReduce::Sum(sm, ParallelContext::CommunicatorSub());

    return sm;
}

void
MultiFab::Add (MultiFab&       dst,
	       const MultiFab& src,
	       int             srccomp,
	       int             dstcomp,
	       int             numcomp,
	       int             nghost)
{
    amrex::Add(dst,src,srccomp,dstcomp,numcomp,nghost);
}

void
MultiFab::Add (MultiFab&       dst,
	       const MultiFab& src,
	       int             srccomp,
	       int             dstcomp,
	       int             numcomp,
	       const IntVect&  nghost)
{
    BL_ASSERT(dst.boxArray() == src.boxArray());
    BL_ASSERT(dst.distributionMap == src.distributionMap);
    BL_ASSERT(dst.nGrowVect().allGE(nghost) && src.nGrowVect().allGE(nghost));

    BL_PROFILE("MultiFab::Add()");

    amrex::Add(dst, src, srccomp, dstcomp, numcomp, nghost);
}

void
MultiFab::Copy (MultiFab&       dst,
                const MultiFab& src,
                int             srccomp,
                int             dstcomp,
                int             numcomp,
                int             nghost)
{
    amrex::Copy(dst,src,srccomp,dstcomp,numcomp,IntVect(nghost));
}

void
MultiFab::Copy (MultiFab&       dst,
                const MultiFab& src,
                int             srccomp,
                int             dstcomp,
                int             numcomp,
                const IntVect&  nghost)
{
// don't have to BL_ASSERT(dst.boxArray() == src.boxArray());
    BL_ASSERT(dst.distributionMap == src.distributionMap);
    BL_ASSERT(dst.nGrowVect().allGE(nghost));

    BL_PROFILE("MultiFab::Copy()");
    
    amrex::Copy(dst,src,srccomp,dstcomp,numcomp,nghost);
}


#ifdef USE_PERILLA
void
MultiFab::Copy (MultiFab&       dst,
                const MultiFab& src,
                int             f,
                int             srccomp,
                int             dstcomp,
                int             numcomp,
                const Box&      bx)
{
// don't have to    BL_ASSERT(dst.boxArray() == src.boxArray());
    BL_ASSERT(dst.distributionMap == src.distributionMap);
    //BL_ASSERT(dst.nGrow() >= nghost); // && src.nGrow() >= nghost);

    int fis = src.IndexArray()[f];
    int fid = dst.IndexArray()[f];
    //const Box& bx = BoxLib::grow(dst[f].box(),nghost);
    //const Box& bx = dst[fid].box();

    if (bx.ok())
      dst[fid].copy(src[fid], bx, srccomp, bx, dstcomp, numcomp);

}
#endif


void
MultiFab::Subtract (MultiFab&       dst,
		    const MultiFab& src,
		    int             srccomp,
		    int             dstcomp,
		    int             numcomp,
		    int             nghost)
{
    Subtract(dst,src,srccomp,dstcomp,numcomp,IntVect(nghost));
}

void
MultiFab::Subtract (MultiFab&       dst,
		    const MultiFab& src,
		    int             srccomp,
		    int             dstcomp,
		    int             numcomp,
		    const IntVect&  nghost)
{
    BL_ASSERT(dst.boxArray() == src.boxArray());
    BL_ASSERT(dst.distributionMap == src.distributionMap);
    BL_ASSERT(dst.nGrowVect().allGE(nghost) && src.nGrowVect().allGE(nghost));

    BL_PROFILE("MultiFab::Subtract()");

    amrex::Subtract(dst,src,srccomp,dstcomp,numcomp,nghost);
}

void
MultiFab::Multiply (MultiFab&       dst,
		    const MultiFab& src,
		    int             srccomp,
		    int             dstcomp,
		    int             numcomp,
		    int             nghost)
{
    Multiply(dst,src,srccomp,dstcomp,numcomp,IntVect(nghost));
}

void
MultiFab::Multiply (MultiFab&       dst,
		    const MultiFab& src,
		    int             srccomp,
		    int             dstcomp,
		    int             numcomp,
		    const IntVect&  nghost)
{
    BL_ASSERT(dst.boxArray() == src.boxArray());
    BL_ASSERT(dst.distributionMap == src.distributionMap);
    BL_ASSERT(dst.nGrowVect().allGE(nghost) && src.nGrowVect().allGE(nghost));

    BL_PROFILE("MultiFab::Multiply()");

    amrex::Multiply(dst,src,srccomp,dstcomp,numcomp,nghost);
}

void
MultiFab::Divide (MultiFab&       dst,
		  const MultiFab& src,
		  int             srccomp,
		  int             dstcomp,
		  int             numcomp,
		  int             nghost)
{
    Divide(dst,src,srccomp,dstcomp,numcomp,IntVect(nghost));
}

void
MultiFab::Divide (MultiFab&       dst,
		  const MultiFab& src,
		  int             srccomp,
		  int             dstcomp,
		  int             numcomp,
		  const IntVect&  nghost)
{
    BL_ASSERT(dst.boxArray() == src.boxArray());
    BL_ASSERT(dst.distributionMap == src.distributionMap);
    BL_ASSERT(dst.nGrowVect().allGE(nghost) && src.nGrowVect().allGE(nghost));

    BL_PROFILE("MultiFab::Divide()");

    amrex::Divide(dst,src,srccomp,dstcomp,numcomp,nghost);
}

void
MultiFab::Saxpy (MultiFab&       dst,
		 Real            a, 
		 const MultiFab& src,
		 int             srccomp,
		 int             dstcomp,
		 int             numcomp,
		 int             nghost)
{
    BL_ASSERT(dst.boxArray() == src.boxArray());
    BL_ASSERT(dst.distributionMap == src.distributionMap);
    BL_ASSERT(dst.nGrow() >= nghost && src.nGrow() >= nghost);

    BL_PROFILE("MultiFab::Saxpy()");

#ifdef _OPENMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
    for (MFIter mfi(dst,TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const Box& bx = mfi.growntilebox(nghost);

        if (bx.ok()) {
            auto const sfab = src.array(mfi);
            auto       dfab = dst.array(mfi);
            AMREX_HOST_DEVICE_PARALLEL_FOR_4D ( bx, numcomp, i, j, k, n,
            {
                dfab(i,j,k,dstcomp+n) += a * sfab(i,j,k,srccomp+n);
            });
        }
    }
}

void
MultiFab::Xpay (MultiFab&       dst,
		Real            a, 
		const MultiFab& src,
		int             srccomp,
		int             dstcomp,
		int             numcomp,
		int             nghost)
{
    BL_ASSERT(dst.boxArray() == src.boxArray());
    BL_ASSERT(dst.distributionMap == src.distributionMap);
    BL_ASSERT(dst.nGrow() >= nghost && src.nGrow() >= nghost);

    BL_PROFILE("MultiFab::Xpay()");

#ifdef _OPENMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
    for (MFIter mfi(dst,TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const Box& bx = mfi.growntilebox(nghost);
        if (bx.ok()) {
            auto const sfab = src.array(mfi);
            auto       dfab = dst.array(mfi);
            AMREX_HOST_DEVICE_PARALLEL_FOR_4D ( bx, numcomp, i, j, k, n,
            {
                dfab(i,j,k,n+dstcomp) = sfab(i,j,k,n+srccomp) + a * dfab(i,j,k,n+dstcomp);
            });
        }
    }
}

void
MultiFab::LinComb (MultiFab&       dst,
		   Real            a,
		   const MultiFab& x,
		   int             xcomp,
		   Real            b,
		   const MultiFab& y,
		   int             ycomp,
		   int             dstcomp,
		   int             numcomp,
		   int             nghost)
{
    BL_ASSERT(dst.boxArray() == x.boxArray());
    BL_ASSERT(dst.distributionMap == x.distributionMap);
    BL_ASSERT(dst.boxArray() == y.boxArray());
    BL_ASSERT(dst.distributionMap == y.distributionMap);
    BL_ASSERT(dst.nGrow() >= nghost && x.nGrow() >= nghost && y.nGrow() >= nghost);

    BL_PROFILE("MultiFab::LinComb()");

#ifdef _OPENMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
    for (MFIter mfi(dst,TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const Box& bx = mfi.growntilebox(nghost);
	
        if (bx.ok()) {
            auto const xfab =   x.array(mfi);
            auto const yfab =   y.array(mfi);
            auto       dfab = dst.array(mfi);
            AMREX_HOST_DEVICE_PARALLEL_FOR_4D ( bx, numcomp, i, j, k, n,
            {
                dfab(i,j,k,dstcomp+n) = a*xfab(i,j,k,xcomp+n) + b*yfab(i,j,k,ycomp+n);
            });
        }
    }
}

void
MultiFab::AddProduct (MultiFab&       dst,
		      const MultiFab& src1,
		      int             comp1,
		      const MultiFab& src2,
		      int             comp2,
		      int             dstcomp,
		      int             numcomp,
		      int             nghost)
{
    BL_ASSERT(dst.boxArray() == src1.boxArray());
    BL_ASSERT(dst.distributionMap == src1.distributionMap);
    BL_ASSERT(dst.boxArray() == src2.boxArray());
    BL_ASSERT(dst.distributionMap == src2.distributionMap);
    BL_ASSERT(dst.nGrow() >= nghost && src1.nGrow() >= nghost && src2.nGrow() >= nghost);

    BL_PROFILE("MultiFab::AddProduct()");

#ifdef _OPENMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
    for (MFIter mfi(dst,TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        const Box& bx = mfi.growntilebox(nghost);
        if (bx.ok()) {
            auto const s1fab = src1.array(mfi);
            auto const s2fab = src2.array(mfi);
            auto        dfab =  dst.array(mfi);
            AMREX_HOST_DEVICE_PARALLEL_FOR_4D ( bx, numcomp, i, j, k, n,
            {
                dfab(i,j,k,n+dstcomp) += s1fab(i,j,k,n+comp1) * s2fab(i,j,k,n+comp2);
            });
        }
    }
}

void
MultiFab::plus (Real val,
                int  nghost)
{
    plus(val,0,n_comp,nghost);
}

void
MultiFab::plus (Real       val,
                const Box& region,
                int        nghost)
{
    plus(val,region,0,n_comp,nghost);
}

void
MultiFab::mult (Real val,
                int  nghost)
{
    mult(val,0,n_comp,nghost);
}

void
MultiFab::mult (Real       val,
                const Box& region,
                int        nghost)
{
    mult(val,region,0,n_comp,nghost);
}

void
MultiFab::invert (Real numerator,
                  int  nghost)
{
    invert(numerator,0,n_comp,nghost);
}

void
MultiFab::invert (Real       numerator,
                  const Box& region,
                  int        nghost)
{
    invert(numerator,region,0,n_comp,nghost);
}

void
MultiFab::negate (int nghost)
{
    negate(0,n_comp,nghost);
}

void
MultiFab::negate (const Box& region,
                  int        nghost)
{
    negate(region,0,n_comp,nghost);
}

void
MultiFab::Initialize ()
{
    if (initialized) return;
    initialized = true;

    amrex::ExecOnFinalize(MultiFab::Finalize);

#ifdef AMREX_MEM_PROFILING
    MemProfiler::add("MultiFab", std::function<MemProfiler::NBuildsInfo()>
		     ([] () -> MemProfiler::NBuildsInfo {
			 return {num_multifabs, num_multifabs_hwm};
		     }));
#endif
}

void
MultiFab::Finalize ()
{
    initialized = false;
}

MultiFab::MultiFab () noexcept
{
#ifdef AMREX_MEM_PROFILING
    ++num_multifabs;
    num_multifabs_hwm = std::max(num_multifabs_hwm, num_multifabs);
#endif
}

MultiFab::MultiFab (const BoxArray&            bxs,
                    const DistributionMapping& dm,
                    int                        ncomp,
                    int                        ngrow,
		    const MFInfo&              info,
                    const FabFactory<FArrayBox>& factory)
    : MultiFab(bxs,dm,ncomp,IntVect(ngrow),info,factory)
{}

MultiFab::MultiFab (const BoxArray&            bxs,
                    const DistributionMapping& dm,
                    int                        ncomp,
                    const IntVect&             ngrow,
		    const MFInfo&              info,
                    const FabFactory<FArrayBox>& factory)
    :
    FabArray<FArrayBox>(bxs,dm,ncomp,ngrow,info,factory)
{
    if (SharedMemory() && info.alloc) initVal();  // else already done in FArrayBox
#ifdef AMREX_MEM_PROFILING
    ++num_multifabs;
    num_multifabs_hwm = std::max(num_multifabs_hwm, num_multifabs);
#endif
}

MultiFab::MultiFab (const MultiFab& rhs, MakeType maketype, int scomp, int ncomp)
    :
    FabArray<FArrayBox>(rhs, maketype, scomp, ncomp)
{
#ifdef AMREX_MEM_PROFILING
    ++num_multifabs;
    num_multifabs_hwm = std::max(num_multifabs_hwm, num_multifabs);
#endif
}

MultiFab::MultiFab (MultiFab&& rhs) noexcept
    : FabArray<FArrayBox>(std::move(rhs))
{
#ifdef AMREX_MEM_PROFILING
    ++num_multifabs;
    num_multifabs_hwm = std::max(num_multifabs_hwm, num_multifabs);
#endif
}

MultiFab::~MultiFab()
{
#ifdef AMREX_MEM_PROFILING
    --num_multifabs;
#endif
}

void
MultiFab::operator= (Real r)
{
    setVal(r);
}

void
MultiFab::define (const BoxArray&            bxs,
                  const DistributionMapping& dm,
                  int                        nvar,
                  int                        ngrow,
		  const MFInfo&              info,
                  const FabFactory<FArrayBox>& factory)
{
    define(bxs, dm, nvar, IntVect(ngrow), info, factory);
    if (SharedMemory() && info.alloc) initVal();  // else already done in FArrayBox
}

void
MultiFab::define (const BoxArray&            bxs,
                  const DistributionMapping& dm,
                  int                        nvar,
                  const IntVect&             ngrow,
		  const MFInfo&              info,
                  const FabFactory<FArrayBox>& factory)
{
    this->FabArray<FArrayBox>::define(bxs,dm,nvar,ngrow,info,factory);
    if (SharedMemory() && info.alloc) initVal();  // else already done in FArrayBox
}

void
MultiFab::initVal ()
{
#ifdef _OPENMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
    for (MFIter mfi(*this, TilingIfNotGPU()); mfi.isValid(); ++mfi)
    {
        FArrayBox& fab = (*this)[mfi];
	fab.initVal();
    }
}

bool 
MultiFab::contains_nan (int scomp,
                        int ncomp,
                        int ngrow,
                        bool local) const
{
    return contains_nan(scomp, ncomp, IntVect(ngrow), local);
}

bool 
MultiFab::contains_nan (int scomp,
                        int ncomp,
                        const IntVect& ngrow,
                        bool local) const
{
    BL_ASSERT(scomp >= 0);
    BL_ASSERT(scomp + ncomp <= nComp());
    BL_ASSERT(ncomp >  0 && ncomp <= nComp());
    BL_ASSERT(IntVect::TheZeroVector().allLE(ngrow) && ngrow.allLE(nGrowVect()));

    bool r = amrex::ReduceLogicalOr(*this, ngrow,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& fab) -> bool
    {
        return fab.contains_nan(bx,scomp,ncomp);
    });

    if (!local) {
        ParallelAllReduce::Or(r, ParallelContext::CommunicatorSub());
    }

    return r;
}

bool 
MultiFab::contains_nan (bool local) const
{
    return contains_nan(0,nComp(),nGrowVect(),local);
}

bool
MultiFab::contains_inf (int scomp,
                        int ncomp,
                        IntVect const& ngrow,
			bool local) const
{
    BL_ASSERT(scomp >= 0);
    BL_ASSERT(scomp + ncomp <= nComp());
    BL_ASSERT(ncomp >  0 && ncomp <= nComp());
    BL_ASSERT(IntVect::TheZeroVector().allLE(ngrow) && ngrow.allLE(nGrowVect()));

    bool r = amrex::ReduceLogicalOr(*this, ngrow,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& fab) -> bool
    {
        return fab.contains_inf(bx,scomp,ncomp);
    });

    if (!local)
	ParallelAllReduce::Or(r, ParallelContext::CommunicatorSub());

    return r;
}

bool 
MultiFab::contains_inf (int scomp,
                        int ncomp,
                        int ngrow,
			bool local) const
{
    return contains_inf(0,ncomp,IntVect(ngrow),local);
}

bool 
MultiFab::contains_inf (bool local) const
{
    return contains_inf(0,nComp(),nGrow(),local);
}

bool 
MultiFab::is_nodal () const noexcept
{
    return boxArray().ixType().nodeCentered();
}

bool 
MultiFab::is_nodal (int dir) const noexcept
{
    return boxArray().ixType().nodeCentered(dir);
}

bool 
MultiFab::is_cell_centered () const noexcept
{
    return boxArray().ixType().cellCentered();
}

Real
MultiFab::min (int comp,
               int nghost,
	       bool local) const
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());

    Real mn = amrex::ReduceMin(*this, nghost,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& fab) -> Real
    {
        return fab.min(bx,comp);
    });

    if (!local)
	ParallelAllReduce::Min(mn, ParallelContext::CommunicatorSub());

    return mn;
}

Real
MultiFab::min (const Box& region,
               int        comp,
               int        nghost,
	       bool       local) const
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());

    Real mn = amrex::ReduceMin(*this, nghost,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& fab) -> Real
    {
        const Box& b = bx & region;
        if (b.ok()) {
            return fab.min(b,comp);
        } else {
#if !defined(__CUDACC__) || (__CUDACC_VER_MAJOR__ != 9) || (__CUDACC_VER_MINOR__ != 2)
            return std::numeric_limits<Real>::max();
#elif defined(BL_USE_DOUBLE)
            return DBL_MAX;
#else
            return FLT_MAX;
#endif
        }
    });

    if (!local)
	ParallelAllReduce::Min(mn, ParallelContext::CommunicatorSub());

    return mn;

}

Real
MultiFab::max (int comp,
               int nghost,
	       bool local) const
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());

    Real mx = amrex::ReduceMax(*this, nghost,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& fab) -> Real
    {
        return fab.max(bx,comp);
    });

    if (!local)
	ParallelAllReduce::Max(mx, ParallelContext::CommunicatorSub());

    return mx;
}

Real
MultiFab::max (const Box& region,
               int        comp,
               int        nghost,
	       bool       local) const
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());

    Real mx = amrex::ReduceMax(*this, nghost,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& fab) -> Real
    {
        const Box& b = bx & region;
        if (b.ok()) {
            return fab.max(b,comp);
        } else {
#if !defined(__CUDACC__) || (__CUDACC_VER_MAJOR__ != 9) || (__CUDACC_VER_MINOR__ != 2)
            return std::numeric_limits<Real>::lowest();
#elif defined(BL_USE_DOUBLE)
            return -DBL_MAX;
#else
            return -FLT_MAX;
#endif
        }
    });

    if (!local)
	ParallelAllReduce::Max(mx, ParallelContext::CommunicatorSub());

    return mx;
}

namespace {

static IntVect
indexFromValue (MultiFab const& mf, int comp, int nghost, Real value, MPI_Op mmloc)
{
    IntVect loc;

#ifdef AMREX_USE_GPU
    if (Gpu::inLaunchRegion())
    {
        int tmp[1+AMREX_SPACEDIM] = {0};
        amrex::Gpu::AsyncArray<int> aa(tmp, 1+AMREX_SPACEDIM);
        int* p = aa.data();
        // This is a device ptr to 1+AMREX_SPACEDIM int zeros.
        // The first is used as an atomic bool and the others for intvect.
        for (MFIter mfi(mf); mfi.isValid(); ++mfi) {
            const Box& bx = amrex::grow(mfi.validbox(), nghost);
            const Array4<Real const> arr = mf.const_array(mfi);
            AMREX_LAUNCH_DEVICE_LAMBDA(bx, tbx,
            {
                int* flag = p;
                if (*flag == 0) {
                    const FArrayBox fab(arr);
                    IntVect t_loc = fab.indexFromValue(value, tbx, comp);
                    if (tbx.contains(t_loc)) {
                        if (Gpu::Atomic::Exch(flag,1) == 0) {
                            AMREX_D_TERM(p[1] = t_loc[0];,
                                         p[2] = t_loc[1];,
                                         p[3] = t_loc[2];);
                        }
                    }
                }
            });
        }
        aa.copyToHost(tmp, 1+AMREX_SPACEDIM);
        AMREX_D_TERM(loc[0] = tmp[1];,
                     loc[1] = tmp[2];,
                     loc[2] = tmp[3];);
    }
    else
#endif
    {
        bool f = false;
#ifdef _OPENMP
#pragma omp parallel
#endif
        {
            IntVect priv_loc = IntVect::TheMinVector();
            for (MFIter mfi(mf,true); mfi.isValid(); ++mfi)
            {
                const Box& bx = mfi.growntilebox(nghost);
                const FArrayBox& fab = mf[mfi];
                IntVect t_loc = fab.indexFromValue(value, bx, comp);
                if (bx.contains(t_loc)) {
                    priv_loc = t_loc;
                };
            }

            if (priv_loc.allGT(IntVect::TheMinVector())) {
                bool old;
// we should be able to test on _OPENMP < 201107 for capture (version 3.1)
// but we must work around a bug in gcc < 4.9
#if defined(_OPENMP) && _OPENMP < 201307 // OpenMP 4.0
#pragma omp critical (amrex_indexfromvalue)
#elif defined(_OPENMP)
#pragma omp atomic capture
#endif
                {
                    old = f;
                    f = true;
                }

                if (old == false) loc = priv_loc;
            }
        }
    }

#ifdef BL_USE_MPI
    const int NProcs = ParallelContext::NProcsSub();
    if (NProcs > 1)
    {
        struct {
            Real mm;
            int rank;
        } in, out;
        in.mm = value;
        in.rank = ParallelContext::MyProcSub();
        MPI_Datatype datatype = (sizeof(Real) == sizeof(double))
            ? MPI_DOUBLE_INT : MPI_FLOAT_INT;
        MPI_Comm comm = ParallelContext::CommunicatorSub();
        MPI_Allreduce(&in,  &out, 1, datatype, mmloc, comm);
        MPI_Bcast(&(loc[0]), AMREX_SPACEDIM, MPI_INT, out.rank, comm);
    }
#endif

    return loc;
}

}

IntVect
MultiFab::minIndex (int comp, int nghost) const
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());
    Real mn = this->min(comp, nghost, true);
    return indexFromValue(*this, comp, nghost, mn, MPI_MINLOC);
}

IntVect
MultiFab::maxIndex (int comp, int nghost) const
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());
    Real mx = this->max(comp, nghost, true);
    return indexFromValue(*this, comp, nghost, mx, MPI_MAXLOC);
}

Real
MultiFab::norm0 (const iMultiFab& mask, int comp, int nghost, bool local) const
{
    Real nm0 = amrex::ReduceMax(*this, mask, nghost,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& fab, IArrayBox const& mskfab)
                                -> Real
    {
        return fab.norminfmask(bx, mskfab, comp, 1);
    });

    if (!local)	ParallelAllReduce::Max(nm0, ParallelContext::CommunicatorSub());

    return nm0;
}

Real
MultiFab::norm0 (int comp, int nghost, bool local) const
{
    Real nm0 = amrex::ReduceMax(*this, nghost,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& fab) -> Real
    {
        return fab.norm(bx, 0, comp, 1);
    });

    if (!local)
	ParallelAllReduce::Max(nm0, ParallelContext::CommunicatorSub());

    return nm0;
}

Vector<Real>
MultiFab::norm0 (const Vector<int>& comps, int nghost, bool local) const
{
    int n = comps.size();
    Vector<Real> nm0;
    nm0.reserve(n);

    for (int comp : comps) {
        nm0.push_back(this->norm0(comp, nghost, true));
    }

    if (!local)
	ParallelAllReduce::Max(nm0.dataPtr(), n, ParallelContext::CommunicatorSub());

    return nm0;
}

Real
MultiFab::norm2 (int comp) const
{
    BL_ASSERT(ixType().cellCentered());

    Real nm2 = MultiFab::Dot(*this, comp, 1, 0);
    nm2 = std::sqrt(nm2);
    return nm2;
}

Real
MultiFab::norm2 (int comp, const Periodicity& period) const
{
    MultiFab tmpmf(boxArray(), DistributionMap(), 1, 0, MFInfo(), Factory());
    MultiFab::Copy(tmpmf, *this, comp, 0, 1, 0);

    auto mask = OverlapMask(period);
    MultiFab::Divide(tmpmf, *mask, 0, 0, 1, 0);

    Real nm2 = MultiFab::Dot(*this, comp, tmpmf, 0, 1, 0);
    nm2 = std::sqrt(nm2);
    return nm2;
}

Vector<Real>
MultiFab::norm2 (const Vector<int>& comps) const
{
    BL_ASSERT(ixType().cellCentered());

    int n = comps.size();
    Vector<Real> nm2;
    nm2.reserve(n);

    for (int comp : comps) {
        nm2.push_back(this->norm2(comp));
    }

    return nm2;
}

Real
MultiFab::norm1 (int comp, const Periodicity& period) const
{
    MultiFab tmpmf(boxArray(), DistributionMap(), 1, 0, MFInfo(), Factory());
    MultiFab::Copy(tmpmf, *this, comp, 0, 1, 0);

    auto mask = OverlapMask(period);
    MultiFab::Divide(tmpmf, *mask, 0, 0, 1, 0);

    return tmpmf.norm1(0, 0);
}

Real
MultiFab::norm1 (int comp, int ngrow, bool local) const
{
    Real nm1 = amrex::ReduceSum(*this, ngrow,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& fab) -> Real
    {
        return fab.norm(bx,1,comp,1);
    });

    if (!local)
	ParallelAllReduce::Sum(nm1, ParallelContext::CommunicatorSub());

    return nm1;
}

Vector<Real>
MultiFab::norm1 (const Vector<int>& comps, int ngrow, bool local) const
{
    BL_ASSERT(ixType().cellCentered());

    int n = comps.size();
    Vector<Real> nm1;
    nm1.reserve(n);

    for (int comp : comps) {
        nm1.push_back(this->norm1(comp, ngrow, true));
    }

    if (!local)
	ParallelAllReduce::Sum(nm1.dataPtr(), n, ParallelContext::CommunicatorSub());

    return nm1;
}

Real
MultiFab::sum (int comp, bool local) const
{
    // 0 ghost cells
    Real sm = amrex::ReduceSum(*this, 0,
    [=] AMREX_GPU_HOST_DEVICE (Box const& bx, FArrayBox const& fab) -> Real
    {
        return fab.sum(bx,comp,1);
    });

    if (!local)
        ParallelAllReduce::Sum(sm, ParallelContext::CommunicatorSub());

    return sm;
}

void
MultiFab::minus (const MultiFab& mf,
                 int             strt_comp,
                 int             num_comp,
                 int             nghost)
{
    MultiFab::Subtract(*this, mf, strt_comp, strt_comp, num_comp, nghost);
}

void
MultiFab::divide (const MultiFab& mf,
		  int             strt_comp,
		  int             num_comp,
		  int             nghost)
{
    MultiFab::Divide(*this, mf, strt_comp, strt_comp, num_comp, nghost);
}

void
MultiFab::plus (Real val,
                int  comp,
                int  num_comp,
                int  nghost)
{

    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());
    BL_ASSERT(comp+num_comp <= n_comp);
    BL_ASSERT(num_comp > 0);

    FabArray<FArrayBox>::plus(val, comp, num_comp, nghost);
}

void
MultiFab::plus (Real       val,
                const Box& region,
                int        comp,
                int        num_comp,
                int        nghost)
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());
    BL_ASSERT(comp+num_comp <= n_comp);
    BL_ASSERT(num_comp > 0);

    FabArray<FArrayBox>::plus(val,region,comp,num_comp,nghost);
}

void
MultiFab::plus (const MultiFab& mf,
                int             strt_comp,
                int             num_comp,
                int             nghost)
{
    MultiFab::Add(*this, mf, strt_comp, strt_comp, num_comp, nghost);
}

void
MultiFab::mult (Real val,
                int  comp,
                int  num_comp,
                int  nghost)
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());
    BL_ASSERT(comp+num_comp <= n_comp);
    BL_ASSERT(num_comp > 0);

    FabArray<FArrayBox>::mult(val,comp,num_comp,nghost);
}

void
MultiFab::mult (Real       val,
                const Box& region,
                int        comp,
                int        num_comp,
                int        nghost)
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());
    BL_ASSERT(comp+num_comp <= n_comp);
    BL_ASSERT(num_comp > 0);

    FabArray<FArrayBox>::mult(val,region,comp,num_comp,nghost);
}

void
MultiFab::invert (Real numerator,
                  int  comp,
                  int  num_comp,
                  int  nghost)
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());
    BL_ASSERT(comp+num_comp <= n_comp);
    BL_ASSERT(num_comp > 0);

    FabArray<FArrayBox>::invert(numerator,comp,num_comp,nghost);
}

void
MultiFab::invert (Real       numerator,
                  const Box& region,
                  int        comp,
                  int        num_comp,
                  int        nghost)
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());
    BL_ASSERT(comp+num_comp <= n_comp);
    BL_ASSERT(num_comp > 0);

    FabArray<FArrayBox>::invert(numerator,region,comp,num_comp,nghost);
}

void
MultiFab::negate (int comp,
                  int num_comp,
                  int nghost)
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());
    BL_ASSERT(comp+num_comp <= n_comp);

    FabArray<FArrayBox>::mult(-1., comp, num_comp, nghost);
}

void
MultiFab::negate (const Box& region,
                  int        comp,
                  int        num_comp,
                  int        nghost)
{
    BL_ASSERT(nghost >= 0 && nghost <= n_grow.min());
    BL_ASSERT(comp+num_comp <= n_comp);

    FabArray<FArrayBox>::mult(-1.,region,comp,num_comp,nghost);
}

void
MultiFab::SumBoundary (int scomp, int ncomp, IntVect const& nghost, const Periodicity& period)
{
    BL_PROFILE("MultiFab::SumBoundary()");

    if ( n_grow == IntVect::TheZeroVector() && boxArray().ixType().cellCentered()) return;

    if (boxArray().ixType().cellCentered()) {
	// Self copy is safe only for cell-centered MultiFab
	this->copy(*this,scomp,scomp,ncomp,n_grow,nghost,period,FabArrayBase::ADD);
    } else {
	MultiFab tmp(boxArray(), DistributionMap(), ncomp, n_grow, MFInfo(), Factory());
	MultiFab::Copy(tmp, *this, scomp, 0, ncomp, n_grow);
	this->setVal(0.0, scomp, ncomp, nghost);
	this->copy(tmp,0,scomp,ncomp,n_grow,nghost,period,FabArrayBase::ADD);
    }
}

void
MultiFab::SumBoundary (int scomp, int ncomp, const Periodicity& period)
{
    SumBoundary(scomp, ncomp, IntVect(0), period);
}

void
MultiFab::SumBoundary (const Periodicity& period)
{
    SumBoundary(0, n_comp, IntVect(0), period);
}

std::unique_ptr<MultiFab>
MultiFab::OverlapMask (const Periodicity& period) const
{
    BL_PROFILE("MultiFab::OverlapMask()");

    const BoxArray& ba = boxArray();
    const DistributionMapping& dm = DistributionMap();

    std::unique_ptr<MultiFab> p{new MultiFab(ba,dm,1,0, MFInfo(), Factory())};

    const std::vector<IntVect>& pshifts = period.shiftIntVect();

#ifdef _OPENMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
    {
        std::vector< std::pair<int,Box> > isects;
        
        for (MFIter mfi(*p); mfi.isValid(); ++mfi)
        {
            const Box& bx = (*p)[mfi].box();
            auto arr = p->array(mfi); 

            AMREX_HOST_DEVICE_PARALLEL_FOR_3D(bx, i, j, k,
            {
                arr(i,j,k) = 0.0;
            });

            for (const auto& iv : pshifts)
            {
                ba.intersections(bx+iv, isects);
                for (const auto& is : isects)
                {
                    Box ibx = is.second-iv;
                    AMREX_HOST_DEVICE_PARALLEL_FOR_3D(ibx, i, j, k,
                    {
                        arr(i,j,k) += 1.0;
                    });
                }
            }
        }
    }
    
    return p;
}


std::unique_ptr<iMultiFab>
MultiFab::OwnerMask (const Periodicity& period) const
{
    return amrex::OwnerMask(*this, period);
}

void
MultiFab::AverageSync (const Periodicity& period)
{
    BL_PROFILE("MultiFab::AverageSync()");

    if (ixType().cellCentered()) return;
    auto wgt = this->OverlapMask(period);
    wgt->invert(1.0, 0, 1);
    this->WeightedSync(*wgt, period);
}

void
MultiFab::WeightedSync (const MultiFab& wgt, const Periodicity& period)
{
    BL_PROFILE("MultiFab::WeightedSync()");

    if (ixType().cellCentered()) return;
    
    const int ncomp = nComp();
    for (int comp = 0; comp < ncomp; ++comp)
    {
        MultiFab::Multiply(*this, wgt, 0, comp, 1, 0);
    }
    
    MultiFab tmpmf(boxArray(), DistributionMap(), ncomp, 0, MFInfo(), Factory());
    tmpmf.setVal(0.0);
    tmpmf.ParallelCopy(*this, period, FabArrayBase::ADD);

    MultiFab::Copy(*this, tmpmf, 0, 0, ncomp, 0);
}

void
MultiFab::OverrideSync (const Periodicity& period)
{
    if (ixType().cellCentered()) return;
    auto msk = this->OwnerMask(period);
    this->OverrideSync(*msk, period);
}

void
MultiFab::OverrideSync (const iMultiFab& msk, const Periodicity& period)
{
    amrex::OverrideSync(*this, msk, period);
}

void
FillBoundary (Vector<MultiFab*> const& mf, const Periodicity& period)
{
    for (auto x : mf) {
        x->FillBoundary(period);
    }
// The following is actually slower on summit
//    Vector<FabArray<FArrayBox>*> fa{mf.begin(),mf.end()};
//    FillBoundary(fa,period);
}

}
