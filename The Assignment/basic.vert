void main()
{
  vec3 x, n, l;
  float ndotl;

  x = vec3(gl_ModelViewMatrix * gl_Vertex);
  n = normalize(gl_NormalMatrix * gl_Normal);
  l = normalize(vec3(gl_LightSource[0].position) - x);
  ndotl = max(dot(n,l),0.0);
  gl_FrontColor = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse * ndotl;

  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

