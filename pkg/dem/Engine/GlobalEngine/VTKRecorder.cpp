#include"VTKRecorder.hpp"
#include<vtkCellArray.h>
#include<vtkPoints.h>
#include<vtkPointData.h>
#include<vtkCellData.h>
#include<vtkSmartPointer.h>
#include<vtkFloatArray.h>
#include<vtkUnstructuredGrid.h>
#include<vtkXMLUnstructuredGridWriter.h>
#include<vtkZLibDataCompressor.h>
//#include<vtkXMLMultiBlockDataWriter.h>
//#include<vtkMultiBlockDataSet.h>
#include<vtkTriangle.h>
#include<vtkLine.h>
#include<yade/core/Scene.hpp>
#include<yade/pkg-common/Sphere.hpp>
#include<yade/pkg-common/Facet.hpp>
#include<yade/pkg-dem/ConcretePM.hpp>


YADE_PLUGIN((VTKRecorder));
YADE_REQUIRE_FEATURE(VTK)
CREATE_LOGGER(VTKRecorder);

void VTKRecorder::action(){
	vector<bool> recActive(REC_SENTINEL,false);
	FOREACH(string& rec, recorders){
		if(rec=="all"){
			recActive[REC_SPHERES]=true;
			recActive[REC_VELOCITY]=true;
			recActive[REC_FACETS]=true;
			recActive[REC_COLORS]=true;
			recActive[REC_INTR]=true;
			recActive[REC_ID]=true;
			recActive[REC_CLUMPID]=true;
			recActive[REC_MATERIALID]=true;
		}
		else if(rec=="spheres") recActive[REC_SPHERES]=true;
		else if(rec=="velocity") recActive[REC_VELOCITY]=true;
		else if(rec=="facets") recActive[REC_FACETS]=true;
		else if(rec=="colors") recActive[REC_COLORS]=true;
		else if(rec=="cpm") recActive[REC_CPM]=true;
		else if(rec=="intr") recActive[REC_INTR]=true;
		else if((rec=="ids") || (rec=="id")) recActive[REC_ID]=true;
		else if((rec=="clumpids") || (rec=="clumpId")) recActive[REC_CLUMPID]=true;
		else if(rec=="materialId") recActive[REC_MATERIALID]=true;
		else LOG_ERROR("Unknown recorder named `"<<rec<<"' (supported are: all, spheres, velocity, facets, colors, cpm, intr, id, clumpId, materialId). Ignored.");
	}
	// cpm needs interactions
	if(recActive[REC_CPM]) recActive[REC_INTR]=true;

	// spheres
	vtkSmartPointer<vtkPoints> spheresPos = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> spheresCells = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkFloatArray> radii = vtkSmartPointer<vtkFloatArray>::New();
	radii->SetNumberOfComponents(1);
	radii->SetName("radii");
	vtkSmartPointer<vtkFloatArray> spheresId = vtkSmartPointer<vtkFloatArray>::New();
	spheresId->SetNumberOfComponents(1);
	spheresId->SetName("id");
	vtkSmartPointer<vtkFloatArray> clumpId = vtkSmartPointer<vtkFloatArray>::New();
	clumpId->SetNumberOfComponents(1);
	clumpId->SetName("clumpId");
	vtkSmartPointer<vtkFloatArray> spheresColors = vtkSmartPointer<vtkFloatArray>::New();
	spheresColors->SetNumberOfComponents(3);
	spheresColors->SetName("colors");
	vtkSmartPointer<vtkFloatArray> spheresVelocity = vtkSmartPointer<vtkFloatArray>::New();
	spheresVelocity->SetNumberOfComponents(3);
	spheresVelocity->SetName("velocity");
	vtkSmartPointer<vtkFloatArray> sphAngVel = vtkSmartPointer<vtkFloatArray>::New();
	sphAngVel->SetNumberOfComponents(3);
	sphAngVel->SetName("angVel");
	vtkSmartPointer<vtkFloatArray> materialId = vtkSmartPointer<vtkFloatArray>::New();
	materialId->SetNumberOfComponents(1);
	materialId->SetName("materialId");
	
	// facets
	vtkSmartPointer<vtkPoints> facetsPos = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> facetsCells = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkFloatArray> facetsColors = vtkSmartPointer<vtkFloatArray>::New();
	facetsColors->SetNumberOfComponents(3);
	facetsColors->SetName("Colors");

	// interactions
	vtkSmartPointer<vtkPoints> intrBodyPos = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> intrCells = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkFloatArray> intrForceN = vtkSmartPointer<vtkFloatArray>::New();
	intrForceN->SetNumberOfComponents(1);
	intrForceN->SetName("forceN");
	vtkSmartPointer<vtkFloatArray> intrAbsForceT = vtkSmartPointer<vtkFloatArray>::New();
	intrAbsForceT->SetNumberOfComponents(3);
	intrAbsForceT->SetName("absForceT");

	// extras for CPM
	if(recActive[REC_CPM]){ CpmStateUpdater csu; csu.update(scene); }
	vtkSmartPointer<vtkFloatArray> cpmDamage = vtkSmartPointer<vtkFloatArray>::New();
	cpmDamage->SetNumberOfComponents(1);
	cpmDamage->SetName("cpmDamage");
	vtkSmartPointer<vtkFloatArray> cpmSigma = vtkSmartPointer<vtkFloatArray>::New();
	cpmSigma->SetNumberOfComponents(3);
	cpmSigma->SetName("cpmSigma");
	vtkSmartPointer<vtkFloatArray> cpmSigmaM = vtkSmartPointer<vtkFloatArray>::New();
	cpmSigmaM->SetNumberOfComponents(1);
	cpmSigmaM->SetName("cpmSigmaM");
	vtkSmartPointer<vtkFloatArray> cpmTau = vtkSmartPointer<vtkFloatArray>::New();
	cpmTau->SetNumberOfComponents(3);
	cpmTau->SetName("cpmTau");

	if(recActive[REC_INTR]){
		// save body positions, referenced by ids by vtkLine
		FOREACH(const shared_ptr<Body>& b, *scene->bodies){
			const Vector3r& pos=b->state->pos;
			intrBodyPos->InsertNextPoint(pos[0],pos[1],pos[2]);
		}
		FOREACH(const shared_ptr<Interaction>& I, *scene->interactions){
			if(!I->isReal()) continue;
			if(skipFacetIntr){
				if(!(dynamic_cast<Sphere*>(Body::byId(I->getId1())->shape.get()))) continue;
				if(!(dynamic_cast<Sphere*>(Body::byId(I->getId2())->shape.get()))) continue;
			}
			vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
			line->GetPointIds()->SetId(0,I->getId1());
			line->GetPointIds()->SetId(1,I->getId2());
			intrCells->InsertNextCell(line);
			if(recActive[REC_CPM]){		//For CPM model 
				const CpmPhys* phys = YADE_CAST<CpmPhys*>(I->interactionPhysics.get());
				intrForceN->InsertNextValue(phys->Fn);
				float fs[3]={abs(phys->shearForce[0]),abs(phys->shearForce[1]),abs(phys->shearForce[2])};
				intrAbsForceT->InsertNextTupleValue(fs);
			} else {									//For all other models
				const NormShearPhys* phys = YADE_CAST<NormShearPhys*>(I->interactionPhysics.get());
				float fn[3]={abs(phys->normalForce[0]),abs(phys->normalForce[1]),abs(phys->normalForce[2])};
				float fs[3]={abs(phys->shearForce[0]),abs(phys->shearForce[1]),abs(phys->shearForce[2])};
				intrForceN->InsertNextTupleValue(fn);
				intrAbsForceT->InsertNextTupleValue(fs);
			}
		}
	}

	FOREACH(const shared_ptr<Body>& b, *scene->bodies){
		if (!b) continue;
		if (recActive[REC_SPHERES]){
			const Sphere* sphere = dynamic_cast<Sphere*>(b->shape.get()); 
			if (sphere){
				if(skipNondynamic && !b->isDynamic) continue;
				vtkIdType pid[1];
				const Vector3r& pos = b->state->pos;
				pid[0] = spheresPos->InsertNextPoint(pos[0], pos[1], pos[2]);
				spheresCells->InsertNextCell(1,pid);
				radii->InsertNextValue(sphere->radius);
				if (recActive[REC_ID]) spheresId->InsertNextValue(b->getId()); 
				if (recActive[REC_CLUMPID]) clumpId->InsertNextValue(b->clumpId);
				if (recActive[REC_COLORS]){
					const Vector3r& color = sphere->color;
					float c[3] = {color[0],color[1],color[2]};
					spheresColors->InsertNextTupleValue(c);
				}
				if(recActive[REC_VELOCITY]){
					const Vector3r& vel = b->state->vel;
					float v[3] = { vel[0],vel[1],vel[2] };
					spheresVelocity->InsertNextTupleValue(v);
					
					const Vector3r& angVel = b->state->angVel;
					float av[3] = { angVel[0],angVel[1],angVel[2] };
					sphAngVel->InsertNextTupleValue(av);
				}
				if (recActive[REC_CPM]){
					cpmDamage->InsertNextValue(YADE_PTR_CAST<CpmState>(b->state)->normDmg);
					const Vector3r& ss=YADE_PTR_CAST<CpmState>(b->state)->sigma;
					const Vector3r& tt=YADE_PTR_CAST<CpmState>(b->state)->tau;
					float s[3]={ss[0],ss[1],ss[2]};
					float t[3]={tt[0],tt[1],tt[2]};
					cpmSigma->InsertNextTupleValue(s);
					cpmSigmaM->InsertNextValue((ss[0]+ss[1]+ss[2])/3.);
					cpmTau->InsertNextTupleValue(t);
				}
				if (recActive[REC_MATERIALID]) materialId->InsertNextValue(b->material->id);
				continue;
			}
		}
		if (recActive[REC_FACETS]){
			const Facet* facet = dynamic_cast<Facet*>(b->shape.get()); 
			if (facet){
				const Se3r& O = b->state->se3;
				const vector<Vector3r>& localPos = facet->vertices;
				Matrix3r facetAxisT=O.orientation.toRotationMatrix();
				vtkSmartPointer<vtkTriangle> tri = vtkSmartPointer<vtkTriangle>::New();
				vtkIdType nbPoints=facetsPos->GetNumberOfPoints();
				for (int i=0;i<3;++i){
					Vector3r globalPos = O.position + facetAxisT * localPos[i];
					facetsPos->InsertNextPoint(globalPos[0], globalPos[1], globalPos[2]);
					tri->GetPointIds()->SetId(i,nbPoints+i);
				}
				facetsCells->InsertNextCell(tri);
				if (recActive[REC_COLORS]){
					const Vector3r& color = facet->color;
					float c[3] = {color[0],color[1],color[2]};
					facetsColors->InsertNextTupleValue(c);
				}
				if (recActive[REC_MATERIALID]) materialId->InsertNextValue(b->material->id);
				continue;
			}
		}
	}

	vtkSmartPointer<vtkDataCompressor> compressor;
	if(compress) compressor=vtkSmartPointer<vtkZLibDataCompressor>::New();

	if (recActive[REC_SPHERES]){
		vtkSmartPointer<vtkUnstructuredGrid> spheresUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
		spheresUg->SetPoints(spheresPos);
		spheresUg->SetCells(VTK_VERTEX, spheresCells);
		spheresUg->GetPointData()->AddArray(radii);
		if (recActive[REC_ID]) spheresUg->GetPointData()->AddArray(spheresId);
		if (recActive[REC_CLUMPID]) spheresUg->GetPointData()->AddArray(clumpId);
		if (recActive[REC_COLORS]) spheresUg->GetPointData()->AddArray(spheresColors);
		if (recActive[REC_VELOCITY]){
			spheresUg->GetPointData()->AddArray(spheresVelocity);
			spheresUg->GetPointData()->AddArray(sphAngVel);
		}
		if (recActive[REC_CPM]){
			spheresUg->GetPointData()->AddArray(cpmDamage);
			spheresUg->GetPointData()->AddArray(cpmSigma);
			spheresUg->GetPointData()->AddArray(cpmSigmaM);
			spheresUg->GetPointData()->AddArray(cpmTau);
		}
		if (recActive[REC_MATERIALID]) spheresUg->GetPointData()->AddArray(materialId);
		
		vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
		if(compress) writer->SetCompressor(compressor);
		string fn=fileName+"spheres."+lexical_cast<string>(scene->currentIteration)+".vtu";
		writer->SetFileName(fn.c_str());
		writer->SetInput(spheresUg);
		writer->Write();
	}
	if (recActive[REC_FACETS]){
		vtkSmartPointer<vtkUnstructuredGrid> facetsUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
		facetsUg->SetPoints(facetsPos);
		facetsUg->SetCells(VTK_TRIANGLE, facetsCells);
		if (recActive[REC_COLORS]) facetsUg->GetCellData()->AddArray(facetsColors);
		if (recActive[REC_MATERIALID]) facetsUg->GetPointData()->AddArray(materialId);
		vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
		if(compress) writer->SetCompressor(compressor);
		string fn=fileName+"facets."+lexical_cast<string>(scene->currentIteration)+".vtu";
		writer->SetFileName(fn.c_str());
		writer->SetInput(facetsUg);
		writer->Write();	
	}
	if (recActive[REC_INTR]){
		vtkSmartPointer<vtkUnstructuredGrid> intrUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
		intrUg->SetPoints(intrBodyPos);
		intrUg->SetCells(VTK_LINE, intrCells);
		if (recActive[REC_CPM]){
			 intrUg->GetCellData()->AddArray(intrForceN);
			 intrUg->GetCellData()->AddArray(intrAbsForceT);
		}
		vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
		if(compress) writer->SetCompressor(compressor);
		string fn=fileName+"intrs."+lexical_cast<string>(scene->currentIteration)+".vtu";
		writer->SetFileName(fn.c_str());
		writer->SetInput(intrUg);
		writer->Write();
	}

	//vtkSmartPointer<vtkMultiBlockDataSet> multiblockDataset = vtkSmartPointer<vtkMultiBlockDataSet>::New();
	//multiblockDataset->SetBlock(0, spheresUg );
	//multiblockDataset->SetBlock(1, facetsUg );
	//vtkSmartPointer<vtkXMLMultiBlockDataWriter> writer = vtkSmartPointer<vtkXMLMultiBlockDataWriter>::New();
	//string fn=fileName+lexical_cast<string>(scene->currentIteration)+".vtm";
	//writer->SetFileName(fn.c_str());
	//writer->SetInput(multiblockDataset);
	//writer->Write();	
}

