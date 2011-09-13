/*
Copyright (c) 2000, The JAP-Team 
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.

	- Redistributions in binary form must reproduce the above copyright notice, 
	  this list of conditions and the following disclaimer in the documentation and/or 
		other materials provided with the distribution.

	- Neither the name of the University of Technology Dresden, Germany nor the names of its contributors 
	  may be used to endorse or promote products derived from this software without specific 
		prior written permission. 

	
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS 
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/

class CATargetInterface
	{
		public:
			CATargetInterface(TargetType target_t,NetworkType net_t,CASocketAddr* p_addr)
				{
					set(target_t,net_t,p_addr);
				}

			CATargetInterface()
				{
					target_type=TARGET_UNKNOWN;
					net_type=UNKNOWN_NETWORKTYPE;
					addr=NULL;
				}

			SINT32 cloneInto(CATargetInterface& oTargetInterface) const
				{
					oTargetInterface.net_type=net_type;
					oTargetInterface.target_type=target_type;
					oTargetInterface.addr=addr->clone();
					return E_SUCCESS;
				}

			SINT32 set(TargetType target_t,NetworkType net_t,CASocketAddr* p_addr)
				{
					target_type=target_t;
					net_type=net_t;
					addr=p_addr;
					return E_SUCCESS;
				}

			SINT32 set(CATargetInterface& source)
				{
					return set(source.target_type,source.net_type,source.addr);
				}

			SINT32 set(CATargetInterface* source)
				{
					return set(source->target_type,source->net_type,source->addr);
				}

			TargetType getTargetType() const
				{
					return target_type;
				}

			CASocketAddr* getAddr() const
				{
					return addr;
				}

			SINT32 cleanAddr()
				{
					delete addr;
					addr=NULL;
					return E_SUCCESS;
				}

		private:
			CASocketAddr* addr;
			TargetType target_type;
			NetworkType net_type;
	};
