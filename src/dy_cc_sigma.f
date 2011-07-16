      Subroutine GetDYCCXsection(IDataSet)
      implicit none
      include 'steering.inc'
      include 'for_debug.inc'
      include 'datasets.inc'
      include 'ntot.inc'
      include 'indata.inc'
      include 'theo.inc'
      include 'fcn.inc'
      integer IDataSet
C-------------------------------------------------
      if (DATASETTheoryType(IDataSet).eq.'kfactor') then
         call GetDYCCXsection_kfactor(IDataSet)
      elseif (DATASETTheoryType(IDataSet).eq.'applgrid') then
         call GetDYXsection_applgrid(IDataSet)         
      endif
      end


      Subroutine GetDYNCXsection(IDataSet)
      implicit none
      include 'steering.inc'
      include 'for_debug.inc'
      include 'datasets.inc'
      include 'ntot.inc'
      include 'indata.inc'
      include 'theo.inc'
      include 'fcn.inc'
      integer IDataSet
C-------------------------------------------------
      if (DATASETTheoryType(IDataSet).eq.'kfactor') then
         call GetDYNCXsection_kfactor(IDataSet)
      elseif (DATASETTheoryType(IDataSet).eq.'applgrid') then
         call GetDYXsection_applgrid(IDataSet)         
      endif
      end

      
      

      Subroutine GetDYXsection_applgrid(IDataSet)         
C---------------------------------------------------
C
C Calculate DY CC cross section using APPLGRID interface.
C
C---------------------------------------------------
      implicit none
      include 'steering.inc'
      include 'for_debug.inc'
      include 'datasets.inc'
      include 'ntot.inc'
      include 'indata.inc'
      include 'theo.inc'

      integer IDataSet      ! data set index
      integer NPMax         ! max. number of DY points
      parameter (NPMax = 200)
      double precision XSec(NPMax) ! applgrid convolution result

      integer i,idx,idxUnit
      double precision TheoryUnit  ! scale factor for theory to bring to data units.

      integer GetInfoIndex         ! function thet returns index of an information string.
C----------------------------------------------------      
      call ag_convolute( DATASETTheoryIndex(IDataSet),XSec)

C check if we have to divide APPLGRID prediction to convert units to data units:
      idxUnit = GetInfoIndex(IDataSet,'theoryunit')
      if (idxUnit.gt.0) then
         Theoryunit = DATASETInfo(idxUnit,IDataSet)
      else
         Theoryunit = 1.
      endif

      do i=1, NDATAPOINTS(IDataSet)
         idx =  DATASETIDX(IDataSet,i)
         THEO(idx) = XSec(i) / TheoryUnit

c         print *,'hady', idx, THEO(idx), DATEN(idx)
      enddo
      end

      Subroutine GetDYCCXsection_kfactor(IDataSet)
C-----------------------------------------------------------
C
C Created by SG, 26/05/2011
C Calculate DY W+, W- and asymmetry cross sections
C
C------------------------------------------------------------
      implicit none
      include 'steering.inc'
      include 'for_debug.inc'
      include 'datasets.inc'
      include 'ntot.inc'
      include 'indata.inc'
      include 'theo.inc'
      include 'fcn.inc'
      integer IDataSet
C-------------------------------------------
      integer NPmax
      parameter(NPmax=100)
      

      double precision wmp_bsigs(NPmax),BinSize


      integer i,idx
      integer idxKfactWplus, idxKfactWminus
      logical LAsymmetry,LWplus
      integer idxBinEta1,idxBinEta2
C Functions:
      integer GetInfoIndex
      integer GetKFactIndex
      integer GetBinIndex
C------------------------------------------------------------

      if (NDATAPOINTS(IDataSet)*2.gt.NPmax) then
         print *,'ERROR IN GetDYCCXsection'
         print *,'INCREASE NPmax to ',NDATAPOINTS(IDataSet)*2
         stop
      endif


c      if (IFlagFCN.eq.1) then
c         call initDYtmp(IDataSet)
c      endif


      call dy_get_res(IDataSet, wmp_bsigs)

      idxKfactWplus  = GetKFactIndex(IDataSet,'Wplus')
      idxKfactWminus = GetKFactIndex(IDataSet,'Wminus') 

C Apply k-factors:
      do i=1,NDATAPOINTS(IDataSet)
         idx =  DATASETIDX(IDataSet,i)
         if (idxKfactWminus.gt.0) then
            wmp_bsigs(2*i-1) = wmp_bsigs(2*i-1)*Kfactors(idxKfactWminus,idx)
         endif
         if (idxKfactWplus.gt.0) then
            wmp_bsigs(2*i) = wmp_bsigs(2*i)*Kfactors(idxKfactWplus ,idx)
         endif
      enddo

C Check type of the data
      LAsymmetry =  
     $     DATASETInfo( GetInfoIndex(IDataSet,'asymmetry'), IDataSet).gt.0

      if (.not. LAsymmetry) then
         LWplus = DATASETInfo( GetInfoIndex(IDataSet,'e charge'), IDataSet).gt.0
C Need also bin sizes:
         idxBinEta1 = GetBinIndex(IDataSet,'eta1')
         idxBinEta2 = GetBinIndex(IDataSet,'eta2')
      endif

      do i=1,NDATAPOINTS(IDataSet)
         idx =  DATASETIDX(IDataSet,i)
         if (LAsymmetry) then
            THEO(idx) = (wmp_bsigs(2*i)-wmp_bsigs(2*i-1) )/(wmp_bsigs(2*i)+wmp_bsigs(2*i-1))
         else
            BinSize = AbstractBins(idxBinEta2,idx)-AbstractBins(idxBinEta1,idx)
            if (LWplus) then
               THEO(idx) = wmp_bsigs(2*i)/BinSize
            else
               THEO(idx) = wmp_bsigs(2*i-1)/BinSize         
            endif
         endif
      enddo
      end


      subroutine  GetDYNCXsection_kfactor(IDataSet)
C-----------------------------------------------------------
C
C Created by SG, 26/05/2011
C C&P from CC by AS 06/07/2011
C
C------------------------------------------------------------
      implicit none
      include 'steering.inc'
      include 'for_debug.inc'
      include 'datasets.inc'
      include 'ntot.inc'
      include 'indata.inc'
      include 'theo.inc'
      include 'fcn.inc'
      integer IDataSet
C-------------------------------------------
      integer NPmax
      parameter(NPmax=100)

      double precision z_bsigs(NPmax),BinSize

      integer i,idx
      integer idxKfactZ
      logical LAsymmetry,LZ
      integer idxBinY1,idxBinY2
C Functions:
      integer GetInfoIndex
      integer GetKFactIndex
      integer GetBinIndex
C------------------------------------------------------------

      if (NDATAPOINTS(IDataSet)*2.gt.NPmax) then
         print *,'ERROR IN GetDYNCXsection'
         print *,'INCREASE NPmax to ',NDATAPOINTS(IDataSet)*2
         stop
      endif


c      if (IFlagFCN.eq.1) then
c         call initDYtmp(IDataSet)
c      endif


      call dy_get_res(IDataSet, z_bsigs)
      idxKfactZ  = GetKFactIndex(IDataSet,'Z0')


C Apply k-factors:
      if (idxKfactZ.gt.0) then
         do i=1,NDATAPOINTS(IDataSet)
            idx =  DATASETIDX(IDataSet,i)
            z_bsigs(i) = z_bsigs(i)*Kfactors(idxKfactZ,idx)
         enddo
      else
         print '(''GetDYNCXsection_kfactor: no kfactor found'')'
      endif


C Need also bin sizes:
      idxBinY1 = GetBinIndex(IDataSet,'y1')
      idxBinY2 = GetBinIndex(IDataSet,'y2')


      do i=1,NDATAPOINTS(IDataSet)
         idx =  DATASETIDX(IDataSet,i)
c         if (LAsymmetry) then
c            THEO(idx) = (wmp_bsigs(2*i)-wmp_bsigs(2*i-1) )/(wmp_bsigs(2*i)+wmp_bsigs(2*i-1))
c         else
            BinSize = AbstractBins(idxBinY2,idx)-AbstractBins(idxBinY1,idx)
            THEO(idx) = z_bsigs(i)/BinSize
c         endif
      enddo
      end


