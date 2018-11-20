#include <AMReX_FilCC_C.H>

namespace amrex {

AMREX_GPU_DEVICE
void
filcc_cell (const IntVect& iv, FArrayBox& dest_fab,
            const int dcomp, const int numcomp,
            GeometryData const& geom, const Real time,
            const BCRec* bcr, const int bcomp,
            const int orig_comp)
{
    const int i = iv[0];
    const auto q = dest_fab.view(iv,dcomp);

    const Box& domain_box = geom.Domain();
    const auto& domain_lo = domain_box.loVect();
    const auto& domain_hi = domain_box.hiVect();
    const int ilo = domain_lo[0];
    const int ihi = domain_hi[0];

    for (int n = 0; n < numcomp; ++n)
    {
        const BCRec& bc = bcr[bcomp+n];

        if (i < ilo)
        {
            if (bc.lo(0) == BCType::foextrap)
            {
                q(0,0,0,n) = q(ilo-i,0,0,n);
            }
            else if (bc.lo(0) == BCType::hoextrap)
            {
                if (i < ilo - 1)
                {
                    q(0,0,0,n) = q(ilo-i,0,0,n);
                }
                // i == ilo-1
                else if (ilo+2 <= ihi)
                {
                    q(0,0,0,n) = 0.125*(15.*q(1,0,0,n) - 10.*q(2,0,0,n) + 3.*q(3,0,0,n));
                }
                else
                {
                    q(0,0,0,n) = 0.5*(3.*q(1,0,0,n) - q(2,0,0,n));
                }
            }
            else if (bc.lo(0) == BCType::reflect_even)
            {
                q(0,0,0,n) = q(2*(ilo-i)-1,0,0,n);
            }
            else if (bc.lo(0) == BCType::reflect_odd)
            {
                q(0,0,0,n) = -q(2*(ilo-i)-1,0,0,n);
            }
        }
        else if (i > ihi)
        {
            if (bc.hi(0) == BCType::foextrap)
            {
                q(0,0,0,n) = q(ihi-i,0,0,n);
            }
            else if (bc.hi(0) == BCType::hoextrap)
            {
                if (i > ihi + 1)
                {
                    q(0,0,0,n) = q(ihi-i,0,0,n);
                }
                // i == ihi+1
                else if (ihi-2 >= ilo)
                {
                    q(0,0,0,n) = 0.125*(15.*q(-1,0,0,n) - 10.*q(-2,0,0,n) + 3.*q(-3,0,0,n));
                }
                else
                {
                    q(0,0,0,n) = 0.5*(3.*q(-1,0,0,n) - q(-2,0,0,n));
                }
            }
            else if (bc.hi(0) == BCType::reflect_even)
            {
                q(0,0,0,n) = q(2*(ihi-i)+1,0,0,n);
            }
            else if (bc.hi(0) == BCType::reflect_odd)
            {
                q(0,0,0,n) = -q(2*(ihi-i)+1,0,0,n);
            }
        }
    }
}

}
