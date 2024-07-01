using net.r_eg.MvsSln;
using net.r_eg.MvsSln.Core;
using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SlnInjectorWPF
{
    public class Solution
    {
        private Sln sln;
        private const SlnItems DefaultItems =
            SlnItems.Projects
            | SlnItems.Header
            | SlnItems.ExtItems
            | SlnItems.SolutionConfPlatforms
            | SlnItems.ProjectConfPlatforms
            | SlnItems.Map
            | SlnItems.SolutionItems
            | SlnItems.ProjectDependenciesXml
            | SlnItems.ProjectDependencies
            | SlnItems.Env;

        private IList<Project> projects = new List<Project>();

        public Solution(string path, SlnItems itemsToLoad = DefaultItems) {
            sln = new Sln(path, itemsToLoad);

            foreach(var projItem in Props.ProjectItems)
            {
                projects.Add(new Project(projItem));
            }
        }

        public bool IsProjectInSlnDir(Project project)
        {
            var slnDir = sln.Result.SolutionDir;
            return project.AbsolutePath.StartsWith(slnDir);
        }

        public void AddProject(Project project)
        {
            var newPath = Path.GetRelativePath(Props.SolutionDir, project.AbsolutePath);

            var newItem = new ProjectItem(project.Props)
            {
                path = newPath
            };

            projects.Add(new Project(newItem));
        }

        public async Task Save(string outPath)
        {
            var slnProto = new LhDataHelper();
            var projectItems = Projects.Select(project => project.Props);

            slnProto
                .SetHeader(Props.Header)
                .SetProjects(projectItems)
                .SetProjectConfigs(Props.ProjectConfigs)
                .SetSolutionConfigs(Props.SolutionConfigs)
                .SetDependencies(Props.ProjectDependencies)
                .SetFolders(Props.SolutionFolders)
                .SetExt(Props.ExtItems);

            using var writer = new SlnWriter(outPath, slnProto);
            await writer.WriteAsync();

        }

        public ISlnResult Props { get { return sln.Result; } }

        public IEnumerable<Project> Projects { get => projects; }
    }
}
