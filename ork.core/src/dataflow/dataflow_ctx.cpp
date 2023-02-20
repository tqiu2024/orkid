////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2022, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
//////////////////////////////////////////////////////////////// 



#include <ork/pch.h>

#include <ork/application/application.h>
#include <ork/dataflow/dataflow.h>
#include <ork/dataflow/scheduler.h>
#include <ork/reflect/properties/register.h>
#include <ork/kernel/orklut.hpp>
#include <ork/reflect/properties/AccessorTyped.hpp>
#include <ork/reflect/properties/DirectTypedMap.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace dataflow {
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
dgregisterblock::dgregisterblock(const std::string& name, int isize)
	: mBlock( isize )
	, mName(name)
{
	for( int io=0; io<isize; io++ )
	{
		mBlock.direct_access(io).mIndex = (isize-1)-io;
		mBlock.direct_access(io).mChildren.clear();
		mBlock.direct_access(io).mpBlock = this;
	}
}
///////////////////////////////////////////////////////////////////////////////
dgregister* dgregisterblock::Alloc()
{
	dgregister* reg = mBlock.allocate();
	OrkAssert( reg != 0 );
	mAllocated.insert(reg);
	return reg;
}
///////////////////////////////////////////////////////////////////////////////
void dgregisterblock::Free( dgregister* preg )
{
	preg->mChildren.clear();
	mBlock.deallocate( preg );
	mAllocated.erase(preg);
}
///////////////////////////////////////////////////////////////////////////////
void dgregisterblock::Clear()
{
	orkvector<dgregister*> deallocvec;

	for( dgregister* reg : mAllocated )
		deallocvec.push_back(reg);

	for( dgregister* reg : deallocvec )
		Free(reg);

}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void dgregister::SetModule( dgmoduleinst_ptr_t pmod )
{	
	if( pmod )
	{
		mpOwner = pmod;

		/////////////////////////////////////////
		// for each output,
		//  find any modules connected to this module
		//  and mark it as a dependency
		/////////////////////////////////////////
		
		int inumouts = pmod->numOutputs();
		for( int io=0; io<inumouts; io++ )
		{	
			outpluginst_ptr_t poutplug = pmod->output(io);
			size_t inumcon = poutplug->numExternalOutputConnections(); 
			
			for( size_t ic=0; ic<inumcon; ic++ )
			{	
				inplugbase* pinp = poutplug->externalOutputConnection(ic);

				if( pinp && pinp->module()!=pmod ) // it is dependent on pmod
				{
					dgmoduleinst_ptr_t childmod = rtti::autocast( pinp->GetModule() );
					mChildren.insert( childmod );
				}
			}
		}
	}
}
//////////////////////////////////
dgregister::dgregister( dgmoduleinst_ptr_t pmod, int idx ) 
	: mIndex(idx)
	, mpOwner(0)
	, mpBlock(nullptr)
{
	SetModule( pmod );
}
///////////////////////////////////////////////////////////////////////////////
// prune no longer needed registers
///////////////////////////////////////////////////////////////////////////////
void dgcontext::Prune(dgmoduleinst_ptr_t pmod) // we are done with pmod, prune registers associated with it
{
	// check all register sets
	for( auto itc : mRegisterSets )
	{
		dgregisterblock* regs = itc.second;
		const orkset<dgregister*>& allocated = regs->Allocated();
		orkvector<dgregister*> deallocvec;

		// check all allocated registers
		for( auto reg : allocated )
		{	
			auto itfind = reg->mChildren.find(pmod);
			
			// were any allocated registers feeding this module?
			// is it also not a probed module ?
			//  if so, they can be pruned!!!
			bool b_didfeed_pmod = (itfind != reg->mChildren.end());
			bool b_is_probed = (reg->mpOwner==mpProbeModule);

			if( b_didfeed_pmod && (false==b_is_probed) )
			{	
				reg->mChildren.erase(itfind);
			}
			if( 0 == reg->mChildren.size() )
			{	
				deallocvec.push_back(reg);
			}
		}
		for( dgregister* free_reg : deallocvec )
		{	regs->Free(free_reg);
		}
	}
}
//////////////////////////////////////////////////////////
void dgcontext::Alloc(outplugbase* poutplug)
{	const std::type_info* tinfo = & poutplug->GetDataTypeId();
	auto itc=mRegisterSets.find( tinfo );
	if( itc != mRegisterSets.end() )
	{	dgregisterblock* regs = itc->second;
		dgregister* preg = regs->Alloc();
		preg->mpOwner = ork::rtti::autocast(poutplug->GetModule());
		poutplug->SetRegister(preg);
	}
}
//////////////////////////////////////////////////////////
void dgcontext::SetRegisters( const std::type_info*pinfo,dgregisterblock* pregs )
{	mRegisterSets[ pinfo ] =  pregs;
}
//////////////////////////////////////////////////////////
dgregisterblock* dgcontext::GetRegisters(const std::type_info*pinfo)
{	auto it=mRegisterSets.find(pinfo);
	return (it==mRegisterSets.end()) ? 0 : it->second;
}
//////////////////////////////////////////////////////////
void dgcontext::Clear()
{	for( auto it : mRegisterSets )
	{	dgregisterblock* pregs = it.second;
		pregs->Clear();
	}
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool dgqueue::IsPending( dgmoduleinst_ptr_t mod)
{	return (pending.find(mod)!=pending.end());
}
//////////////////////////////////////////////////////////
int dgqueue::NumDownstream( dgmoduleinst_ptr_t mod )
{	int inumoutcon = 0;
	int inumouts = mod->GetNumOutputs();
	for( int io=0; io<inumouts; io++ )
	{	outpluginst_ptr_t poutplug = mod->output(io);
		inumoutcon+=(int)poutplug->GetNumExternalOutputConnections();
	}
	return inumoutcon;		
}
//////////////////////////////////////////////////////////
int dgqueue::NumPendingDownstream( dgmoduleinst_ptr_t mod )
{	int inumoutcon = 0;
	int inumouts = mod->GetNumOutputs();
	for( int io=0; io<inumouts; io++ )
	{	outpluginst_ptr_t poutplug = mod->output(io);
		size_t inumcon = poutplug->GetNumExternalOutputConnections();
		for( size_t ic=0; ic<inumcon; ic++ )
		{	inplugbase* pinplug = poutplug->GetExternalOutputConnection(ic);
			dgmoduleinst_ptr_t pconmod = rtti::autocast(pinplug->GetModule());
			inumoutcon += int(IsPending(pconmod));
		}
	}
	return inumoutcon;		
}
//////////////////////////////////////////////////////////
void dgqueue::AddModule( dgmoduleinst_ptr_t mod )
{	mod->Key().mDepth=0;
	mod->Key().mSerial = -1;
	int inumo = mod->GetNumOutputs();
	mod->Key().mModifier = s8(-inumo);
	for( int io=0; io<inumo; io++ )
	{
		mod->output(io)->SetRegister(0);
	}
	pending.insert(mod);
}
//////////////////////////////////////////////////////////
void dgqueue::PruneRegisters(dgmoduleinst_ptr_t pmod )
{	mCompCtx.Prune(pmod);
}
//////////////////////////////////////////////////////////
void dgqueue::QueModule( dgmoduleinst_ptr_t pmod, int irecd )
{	
	if( pending.find(pmod) != pending.end() ) // is pmod pending ?
	{	
		if( mModStack.size() )
		{	// check the top of stack for registers to prune
			dgmoduleinst_ptr_t prev = mModStack.top();
			PruneRegisters(prev);
		}
		mModStack.push(pmod);
		///////////////////////////////////
		pmod->Key().mSerial = s8(mSerial++);
		pending.erase(pmod);
		///////////////////////////////////
		int inuminps = pmod->GetNumInputs();
		int inumouts = pmod->GetNumOutputs();
		///////////////////////////////////
		// assign new registers
		///////////////////////////////////
		int inumincon = 0;
		for( int ii=0; ii<inuminps; ii++ )
		{	inplugbase* pinpplug = pmod->GetInput(ii);
			inumincon += int( pinpplug->IsConnected() );
		}
		for( int io=0; io<inumouts; io++ )
		{	outplugbase* poutplug = pmod->output(io);
			if( poutplug->IsConnected() || (inumincon!=0) ) // if it has input or output connections
			{	
				mCompCtx.Alloc(poutplug);
			}
		}
		///////////////////////////////////
		// add dependants to register 
		///////////////////////////////////
		for( int io=0; io<inumouts; io++ )
		{	outplugbase* poutplug = pmod->output(io);
			dgregister* preg = poutplug->GetRegister();
			if( preg )
			{	size_t inumcon = poutplug->GetNumExternalOutputConnections(); 
				for( size_t ic=0; ic<inumcon; ic++ )
				{	inplugbase* pinp = poutplug->GetExternalOutputConnection(ic);
					if( pinp && pinp->GetModule()!=pmod )
					{	dgmoduleinst_ptr_t dmod = rtti::autocast( pinp->GetModule() );
						preg->mChildren.insert(dmod);
					}
				}
			}
		}
		///////////////////////////////////
		// completed "pmod"
		//  add connected with no other pending deps
		///////////////////////////////////
		for( int io=0; io<inumouts; io++ )
		{	outpluginst_ptr_t poutplug = pmod->output(io);
			if( poutplug->GetRegister() )
			{	size_t inumcon = poutplug->GetNumExternalOutputConnections(); 
				for( size_t ic=0; ic<inumcon; ic++ )
				{	inplugbase* pinp = poutplug->GetExternalOutputConnection(ic);
					if( pinp && pinp->GetModule()!=pmod )
					{	dgmoduleinst_ptr_t dmod = rtti::autocast( pinp->GetModule() );
						if( false == HasPendingInputs( dmod ) )
						{	QueModule( dmod, irecd+1 );
						}
					}
				}
			}
		}
		if( pending.size() != 0 )
		{
			PruneRegisters(pmod);
		}
		///////////////////////////////////
		mModStack.pop();
	}
}
//////////////////////////////////////////////////////////
void dgqueue::DumpOutputs( dgmoduleinst_ptr_t mod ) const
{	int inump = mod->GetNumOutputs();
	for( int ip=0; ip<inump; ip++ )
	{	outpluginst_ptr_t poutplug = mod->output(ip);
		dgregister* preg = poutplug->GetRegister();	
		dgregisterblock* pblk = (preg!=nullptr) ? preg->mpBlock : nullptr;
		std::string regb = (pblk!=nullptr) ? pblk->GetName() : "";
		int reg_index = (preg!=nullptr) ? preg->mIndex : -1;		
		printf( "  mod<%s> out<%d> reg<%s:%d>\n", mod->GetName().c_str(), ip, regb.c_str(), reg_index );
	}
}
void dgqueue::DumpInputs( dgmoduleinst_ptr_t mod ) const
{	int inumins = mod->GetNumInputs();
	for( int ip=0; ip<inumins; ip++ )
	{	const inplugbase* pinplug = mod->GetInput(ip);
		if( pinplug->GetExternalOutput() )
		{	outpluginst_ptr_t poutplug = pinplug->GetExternalOutput();
			dgmoduleinst_ptr_t pconcon = rtti::autocast(poutplug->GetModule());
			dgregister* preg = poutplug->GetRegister();	
			if( preg )
			{
				dgregisterblock* pblk = preg->mpBlock;
				std::string regb = (pblk!=nullptr) ? pblk->GetName() : "";
				int reg_index = preg->mIndex;		
				printf( "  mod<%s> inp<%d> -< module<%s> reg<%s:%d>\n", mod->GetName().c_str(), ip, pconcon->GetName().c_str(), regb.c_str(), reg_index );
			}
		}
	}
}
bool dgqueue::HasPendingInputs( dgmoduleinst_ptr_t mod )
{	bool bhaspending = false;
	int inumins = mod->GetNumInputs();
	for( int ip=0; ip<inumins; ip++ )
	{	const inplugbase* pinplug = mod->GetInput(ip);
		if( pinplug->GetExternalOutput() )
		{	outpluginst_ptr_t pout = pinplug->GetExternalOutput();
			dgmoduleinst_ptr_t pconcon = rtti::autocast(pout->GetModule());
			std::set<dgmoduleinst_ptr_t>::iterator it = pending.find(pconcon);
			if( pconcon == mod && typeid(float)==pinplug->GetDataTypeId() ) // connected to self and a float plug, must be an internal loop rate plug
			{	//pending.erase(it);
				//it = pending.end();
			}
			else if( it != pending.end() )
			{	bhaspending = true;
			}
		}
	}
	return bhaspending;
}
//////////////////////////////////////////////////////////
dgqueue::dgqueue( const graph_inst* pg, dgcontext& ctx )
	: mSerial(0)
	, mCompCtx( ctx )
{	/////////////////////////////////////////
	// add all modules
	/////////////////////////////////////////
	size_t inumchild = pg->GetNumChildren();
	for( size_t ic=0; ic<inumchild; ic++ )
	{	dgmoduleinst_ptr_t pmod = pg->GetChild((int)ic);
		AddModule( pmod );	
	}
	/////////////////////////////////////////
	// compute depths iteratively 
	/////////////////////////////////////////
	int inumchg = -1;
	while( inumchg != 0 )
	{	inumchg = 0;
		for( size_t ic=0; ic<inumchild; ic++ )
		{	dgmoduleinst_ptr_t pmod = pg->GetChild(ic);
			int inumouts = pmod->GetNumOutputs();
			for( int op=0; op<inumouts; op++ )
			{	outpluginst_ptr_t poutplug = pmod->output(op);
				size_t inumcon = poutplug->GetNumExternalOutputConnections();
				int ilo = 0;
				for( size_t ic=0; ic<inumcon; ic++ )
				{	inplugbase* pin = poutplug->GetExternalOutputConnection(ic);
					dgmoduleinst_ptr_t pcon = rtti::autocast(pin->GetModule());
					int itd = pcon->Key().mDepth-1;
					if( itd < ilo ) ilo = itd;
				}
				if( pmod->Key().mDepth > ilo && ilo!=0)
				{	pmod->Key().mDepth = s8(ilo);
					inumchg++;
				}
			}
			//printf( " mod<%s> comp_depth<%d>\n", pmod->GetName().c_str(), pmod->Key().mDepth );
					
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} }
///////////////////////////////////////////////////////////////////////////////
