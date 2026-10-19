"""Read mesh bounds/materials and PCGEx authoring API without saving assets."""
import unreal as ue
for name in ['SM_FencePart_01','SM_Fence_01','SM_Fence05','SM_Fence09','SM_Container01_01','SM_Container01_02']:
    obj=ue.load_asset('/Game/EclipseRecoil/Art/FactoryEnvironment/Meshes/'+name)
    if obj:
        ue.log('ER_INDUSTRIAL_ASSET '+name+' bounds='+str(obj.get_bounding_box())+' materials='+str([str(x.material_interface) for x in obj.static_materials]))
for name in ['PCGGraph','PCGLoadDataAssetSettings','PCGExPathPropertiesSettings','PCGDataAsset']:
    cls=getattr(ue,name,None)
    ue.log('ER_PCG_API '+name+' '+str(cls.__doc__ if cls else 'absent'))
