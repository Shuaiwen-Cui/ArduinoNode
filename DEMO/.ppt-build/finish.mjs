import path from 'node:path';
import fs from 'node:fs/promises';
import {pathToFileURL} from 'node:url';
const D='E:/PROJ/ArduinoNode/DEMO/.ppt-build';
const S='C:/Users/cswof/.codex/plugins/cache/openai-primary-runtime/presentations/26.921.10847/skills/presentations';
const {finalizePresentation}=await import(pathToFileURL(S+'/container_tools/artifact_tool_utils.mjs'));
const {tableOwners}=JSON.parse(await fs.readFile(D+'/slides.json','utf8'));
const result=await finalizePresentation({workspaceDir:D,candidatePath:D+'/candidate.pptx',finalPath:D+'/output/ArduinoNode_NTU_Student_Final_v6.pptx',pythonExecutable:'C:/Users/cswof/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe',integrityValidatorPath:S+'/container_tools/inspect_presentation_package_integrity.py',layoutValidatorPath:S+'/container_tools/inspect_presentation_layout_geometry.py',layoutArgs:['--expected-slide-size-emu','12192000,6858000','--validate-bullet-geometry','--validate-heading-fit',...tableOwners.flatMap(n=>['--require-native-table-slide',String(n)])],requiredNativeTableOwnerSlides:tableOwners,fontPolicy:{basis:'design',families:['Arial','Consolas']},verifyArtifactToolImport:true,receiptPath:D+'/validation-student-v6.json'});
console.log(JSON.stringify(result));

